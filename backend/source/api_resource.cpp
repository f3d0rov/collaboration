
#include "api_resource.hpp"

ApiResponse::ApiResponse () {

}

ApiResponse::ApiResponse (int status):
status (status) {

}

ApiResponse::ApiResponse (nlohmann::json body, int status):
body (body), status (status) {

}

std::string ApiResponse::getBody () {
	return this->body.dump();
}

std::string ApiResponse::getMime () {
	return "application/json";
}


ApiResource::ApiResource (mg_context* ctx, std::string uri):
Resource (ctx, uri) {
	logger << "Интерфейс API: \"/" << uri << "\"" << std::endl;
}

std::unique_ptr<_Response> ApiResource::processRequest (RequestData &rd) {
	if (rd.body != "") {
		nlohmann::json json;
		try {
			json = nlohmann::json::parse (rd.body);
		} catch (nlohmann::json::exception &e) {
			return std::make_unique<Response> ("Bad JSON", 400);
		}
		return this->processRequest (rd, json);
	}
	return this->processRequest (rd, nlohmann::json{});

}
