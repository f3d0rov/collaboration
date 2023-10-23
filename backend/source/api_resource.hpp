#pragma once

#include <initializer_list>

#include "civetweb/CivetServer.h"
#include "nlohmann-json/json.hpp"

#include "utils.hpp"
#include "resource.hpp"

class ApiResponse;
class ApiResource;


class ApiResponse {
	private:
		std::vector <std::pair <std::string, std::string>> _customHeaders;
	public:
		nlohmann::json body;
		int status = 500;

		ApiResponse ();
		ApiResponse (int status);
		ApiResponse (nlohmann::json body, int status = 200);

		operator Response ();

		ApiResponse& addHeader (std::string name, std::string value);
		ApiResponse& setCookie (std::string name, std::string value, bool httpOnly = false, long long maxAge = 60 * 60 * 24 * 7);
		const std::vector <std::pair<std::string, std::string>> &headers();
};


class ApiResource: public Resource {
	public:
		ApiResource (mg_context* ctx, std::string uri);
		Response processRequest (RequestData &rd) final;

		virtual ApiResponse processRequest(RequestData &rd, nlohmann::json body) = 0;
};
