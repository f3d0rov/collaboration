
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
	return Response (this->body.dump(), "application/json", this->status, this->_customHeaders);
}

ApiResponse& ApiResponse::addHeader (std::string name, std::string value) {
	this->_customHeaders.push_back (std::make_pair (name, value));
	return *this;
}

ApiResponse& ApiResponse::setCookie (std::string name, std::string value, bool httpOnly, long long maxAge) {
	this->addHeader (
		"Set-Cookie",
		cookieString (name, value, httpOnly, maxAge)
	);
	return *this;
}

const std::vector <std::pair<std::string, std::string>>& ApiResponse::headers() {
	return this->_customHeaders;
}



ApiResource::ApiResource (mg_context* ctx, std::string uri):
Resource (ctx, uri) {
	logger << "Интерфейс API: \"/" << uri << "\"" << std::endl;
}


Response ApiResource::processRequest (RequestData &rd) {
	if (rd.body != "") {
		nlohmann::json json;
		try {
			json = nlohmann::json::parse (rd.body);
		} catch (nlohmann::json::exception &e) {
			return Response ("Bad JSON", 400);
		}
		return this->processRequest (rd, json);
	}
	return this->processRequest (rd, nlohmann::json{});

}
