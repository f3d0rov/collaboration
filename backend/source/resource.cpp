
#include "resource.hpp"


_Response::_Response () {

}

_Response::~_Response () {

}

_Response& _Response::addHeader (std::string name, std::string value) {
	this->_customHeaders.push_back (std::make_pair (name, value));
	return *this;
}

_Response& _Response::setCookie (std::string name, std::string value, bool httpOnly, long long maxAge) {
	this->addHeader (
		"Set-Cookie",
		cookieString (name, value, httpOnly, maxAge)
	);
	return *this;
}

const std::vector <std::pair<std::string, std::string>>& _Response::headers() {
	return this->_customHeaders;
}

void _Response::redirect (std::string address) {
	this->addHeader (
		"Location",
		address.c_str()
	);
	this->status = 303;
}



Response::Response (int status):
status (status) {

}

Response::Response (std::string body, int status):
body (body), status (status) {

}

Response::Response (std::string body, std::string mimeType, int status):
body (body), status (status), mime (mimeType) {

}

Response::~Response () {

}

std::string Response::getBody () {
	return this->body;
}

std::string Response::getMime () {
	return this->mime;
}


void RequestData::setCookiesFromString (const char* cookies_) {
	this->setCookies = std::map <std::string, std::string>();
	if (cookies_ == nullptr) return;
	std::string cookies (cookies_);
	
	bool last = false;
	do {
		int nextSemicolon = cookies.find_first_of (';');

		if (nextSemicolon == cookies.npos) {
			nextSemicolon = cookies.length ();
			last = true;
		}
		std::string nameValuePair = cookies.substr (0, nextSemicolon);
		int eq = nameValuePair.find ('=');
		std::string name = trimmed(nameValuePair.substr (0, eq));
		std::string value = trimmed(nameValuePair.substr (eq + 1));
		this->setCookies [name] = value;

		if (!last) cookies = cookies.substr (nextSemicolon + 1); // Inefficient?
	} while (!last);
}

void RequestData::setQueryVariables (const char* queryString) {
	if (queryString == nullptr) return;
	std::string queries (queryString);

	bool last = false;
	do {
		int nextSemicolon = queries.find_first_of (';');

		if (nextSemicolon == queries.npos) {
			nextSemicolon = queries.length ();
			last = true;
		}
		std::string nameValuePair = queries.substr (0, nextSemicolon);
		int eq = nameValuePair.find ('=');
		std::string name = trimmed(nameValuePair.substr (0, eq));
		std::string value = trimmed(nameValuePair.substr (eq + 1));
		this->query [name] = value;

		if (!last) queries = queries.substr (nextSemicolon + 1); // Inefficient?
	} while (!last);
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
		
		RequestData rd;
		rd.body = body;
		rd.method = method;
		rd.uri = uri;
		rd.ip = ri->remote_addr;

		const char *cookie = mg_get_header(conn, "Cookie");
		rd.setCookiesFromString (cookie);
		rd.setQueryVariables(ri->query_string);
		// logger << "Cookie: " << cookie << std::endl;

		auto response = this->processRequest (rd);
		logger << "Ответ: " << response->status << std::endl;
		
		mg_response_header_start (conn, response->status);
		for (auto &i: response->headers()) {
			mg_response_header_add (conn, i.first.c_str(), i.second.c_str(), i.second.length());
		}

		std::string contentType = response->getMime() + "; charset=utf-8";
		mg_response_header_add (conn, "Content-Type", contentType.c_str(), contentType.length());
		mg_response_header_send (conn);

		std::string respBody = response->getBody();
		mg_write (conn, respBody.c_str(), respBody.length());
		return response->status;

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

std::unique_ptr<_Response> Resource::processRequest (RequestData &rd) {
	return std::make_unique<Response> (404);
}

std::string_view Resource::uri () {
	return this->_uri;
}
