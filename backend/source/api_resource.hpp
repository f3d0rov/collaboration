#pragma once

#include <initializer_list>

#include "civetweb/CivetServer.h"
#include "nlohmann-json/json.hpp"

#include "utils.hpp"
#include "resource.hpp"

class ApiResponse;
class ApiResource;


class ApiResponse {
	public:
		nlohmann::json body;
		int status = 500;

		ApiResponse ();
		ApiResponse (int status);
		ApiResponse (nlohmann::json body, int status = 200);

		operator Response ();
};


class ApiResource: public Resource {
	public:
		ApiResource (mg_context* ctx, std::string uri);
		Response processRequest (std::string method, std::string uri, std::string body) final;

		virtual ApiResponse processRequest(std::string method, std::string uri, nlohmann::json body) = 0;
};
