
#include "api_resource.hpp"

ApiResponse::ApiResponse () {

}

ApiResponse::ApiResponse (int status):
status (status) {

}

ApiResponse::ApiResponse (nlohmann::json body, int status):
body (body), status (status) {

}

ApiResponse::operator Response () {
	return Response (this->body.dump(), "application/json", this->status);
}

ApiResource::ApiResource (mg_context* ctx, std::string uri):
Resource (ctx, uri) {
	logger << "Интерфейс API: \"/" << uri << "\"" << std::endl;
}

Response ApiResource::processRequest (std::string method, std::string uri, std::string body) {

	if (body != "") {
		nlohmann::json json;
		try {
			json = nlohmann::json::parse (body);
		} catch (nlohmann::json::exception &e) {
			return Response ("Bad JSON", 400);
		}
		return this->processRequest (method, uri, json);
	}
	return this->processRequest (method, uri, nlohmann::json{});

}
