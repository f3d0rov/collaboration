#pragma once

#include <initializer_list>
#include <memory>

#include "civetweb/CivetServer.h"
#include "nlohmann-json/json.hpp"

#include "utils.hpp"
#include "resource.hpp"

class ApiResponse;
class ApiResource;


class ApiResponse: public _Response {
	public:
		nlohmann::json body;

		ApiResponse ();
		ApiResponse (int status);
		ApiResponse (nlohmann::json body, int status = 200);
		~ApiResponse ();

		std::string getBody () final;
		std::string getMime () final;
};


class ApiResource: public Resource {
	public:
		ApiResource (mg_context* ctx, std::string uri);
		std::unique_ptr<_Response> processRequest (RequestData &rd) final;

		virtual std::unique_ptr<ApiResponse> processRequest(RequestData &rd, nlohmann::json body) = 0;
};
