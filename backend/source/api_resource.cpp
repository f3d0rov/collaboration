
#include "api_resource.hpp"

ApiResponse::ApiResponse () {

}

ApiResponse::ApiResponse (int status) {
	this->status = status;
}

ApiResponse::ApiResponse (nlohmann::json body, int status):
body (body) {
	this->status = status;
}

ApiResponse::~ApiResponse () {

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
	nlohmann::json json{};
	if (rd.body != "") {
		try {
			json = nlohmann::json::parse (rd.body);
		} catch (nlohmann::json::exception &e) {
			logger << this->uri() << ": Bad JSON" << std::endl;
			return std::make_unique<Response> ("Bad JSON", 400);
		}
	}
	auto resp = this->processRequest (rd, json);
	// logger << this->uri() << ": " << resp->getBody() << std::endl;
	return resp;
}
