#pragma once

#include <initializer_list>
#include <memory>

#include "civetweb/CivetServer.h"
#include "nlohmann-json/json.hpp"

#include "utils.hpp"
#include "resource.hpp"

class ApiResponse;
class ApiResource;


void assertMethod (const RequestData &rd, std::string method);


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

typedef std::unique_ptr <ApiResponse> ApiResponsePtr;

template <class ...Args>
ApiResponsePtr makeApiResponse (Args ...args) {
	return std::unique_ptr <ApiResponse> (new ApiResponse (args...));
}


class ApiResource: public Resource {
	public:
		ApiResource (mg_context* ctx, std::string uri);
		virtual ~ApiResource ();
		std::unique_ptr<_Response> processRequest (RequestData &rd) final;

		virtual ApiResponsePtr processRequest(RequestData &rd, nlohmann::json body) = 0;
};


template <class T>
T getParameter (std::string name, const nlohmann::json &j) {
	if (!j.contains (name)) throw UserMistakeException ("В json-объекте отсутствует поле '"s + name + "'", 400, "bad_json");
	try {
		return j.at(name).get <T> ();
	} catch (nlohmann::json::exception &e) {
		throw UserMistakeException ("Невозможно привести к корректному типу поле '"s + name + "'", 400, "bad_json");
	}
}
