
#include "api_resource.hpp"


void assertMethod (const RequestData &rd, std::string method) {
	if (rd.method != method) throw UserMistakeException ("Method Not Allowed", 405, "method_not_allowed");
}




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

ApiResource::~ApiResource () = default;


std::unique_ptr<_Response> ApiResource::processRequest (RequestData &rd) {
	nlohmann::json json{};
	if (rd.body != "") {
		try {
			json = nlohmann::json::parse (rd.body);
		} catch (nlohmann::json::exception &e) {
			if (common.logApiOut) logger << "[API OUT] Bad JSON" << std::endl;
			return std::make_unique<Response> ("Bad JSON", 400);
		}
	}

	if (common.logApiIn) logger << "[API IN] " << json.dump(2) << std::endl;
	try {
		auto resp = this->processRequest (rd, json);
		if (common.logApiOut) logger << "[API OUT] " << resp->body.dump(2) << std::endl;
		return resp;
	} catch (UserMistakeException &e) {
		auto resp = makeApiResponse (nlohmann::json {{"error", e.what()}}, e.statusCode());
		if (e.errorCode() != "") resp->body["error_code"] = e.errorCode();
		if (common.logApiOut) logger << "[API OUT] " << resp->body.dump(2) << std::endl;
		return resp;
	}
	// logger << this->uri() << ": " << resp->getBody() << std::endl;
}
