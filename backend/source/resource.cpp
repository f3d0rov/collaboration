
#include "resource.hpp"

Response::Response (int status):
status (status) {

}

Response::Response (std::string body, int status):
body (body), status (status) {

}

Response::Response (std::string body, std::string mimeType, int status):
body (body), status (status), mime (mimeType) {

}


Resource::Resource (mg_context* ctx, std::string uri):
_uri (uri) {
	mg_set_request_handler (
		ctx, 
		(std::string ("/") + uri).c_str(),
		Resource::_staticProcessRequest,
		this // mg_set_request_handler requires static function as 3rd argument and accepts no `bind`s.
			// We use cbdata argument (which is a user-provided void* pointer) to remind each resource its address
	);
}

int Resource::_staticProcessRequest (mg_connection* conn, void *cbdata) {
	return ((Resource*) cbdata)->_processRequest (conn);
}

std::string Resource::_readRequestBody (mg_connection* conn) {
	std::string body = "";
	char *buffer = new char[BUFFER_SIZE];
	int dlen = 0;

	do {
		dlen = mg_read (conn, buffer, BUFFER_SIZE);
		body += std::string (buffer, dlen);
	} while (dlen == BUFFER_SIZE); // Read full buffer, means more to come

	delete[] buffer;
	return body;
}

int Resource::_processRequest (mg_connection* conn) {
	if (conn == nullptr) {
		logger << "mg_connection is NULL while processing request to " << "/" << this->_uri << std::endl;
		logger << "Dropping request." << std::endl;
		return 0;
	}

	const mg_request_info *ri = mg_get_request_info(conn);
	std::string method = ri->request_method;
	std::string uri = ri->request_uri;

	try {
		std::string body = this->_readRequestBody (conn);

		// Report incoming connection
		logger << method << " " << uri << " from " << ri->remote_addr << ":" << ri->remote_port << ", body: " << body.length() << " bytes" << std::endl;
		
		Response response = this->processRequest (method, uri, body);
		
		if (response.status == 200) {
			std::string header = response.mime + "; charset=utf-8";
			mg_send_http_ok(conn, header.c_str(), response.body.length());
			mg_write (conn, response.body.c_str(), response.body.length());
			// this->reportRequest (
			// 	response, uri, method, std::string (ri->remote_addr) + ":" + std::to_string(ri->remote_port), body.length(), usElapsedFrom_hiRes(start)
			// );
			return 200;
		} else {
			mg_send_http_error (conn, response.status, "%s", response.body.c_str());
		}
		// this->reportRequest (
		// 	response, uri, method, std::string (ri->remote_addr) + ":" + std::to_string(ri->remote_port), body.length(), usElapsedFrom_hiRes(start)
		// );
		return response.status;	
	} catch (std::exception& e) {
		logger << "Internal error while processing request: " << e.what() << std::endl;
		std::string response = "Server had an unexpected error while processing the request.";
		mg_send_http_error (conn, 500, "%s", response.c_str());
		// this->reportRequest (
		// 	Response (response, 500), uri, method, std::string (ri->remote_addr) + ":" + std::to_string(ri->remote_port), -1, usElapsedFrom_hiRes(start)
		// );
		return 500;
	}

	return 0;
}

Response Resource::processRequest (std::string method, std::string uri, std::string request) {
	return Response (404);
}

std::string_view Resource::uri () {
	return this->_uri;
}
