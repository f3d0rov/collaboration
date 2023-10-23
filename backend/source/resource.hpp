#pragma once

#include <string>

#include "civetweb/CivetServer.h"

#include "utils.hpp"

#define BUFFER_SIZE 2048

class Response;
class Resource;

class Response {
	public:
		std::string body = "";
		std::string mime = "text/html";
		int status = 200;
		std::vector <std::pair<std::string, std::string>> _customHeaders;

		Response (int status);
		Response (std::string body, int status = 200);
		Response (std::string body, std::string mimeType, int status = 200);
		Response (std::string body, std::string mimeType, int status, const std::vector<std::pair<std::string, std::string>>& headers);

		Response& addHeader (std::string name, std::string value);
		Response& setCookie (std::string name, std::string value, bool httpOnly = false, long long maxAge = 60 * 60 * 24 * 7);
		const std::vector <std::pair<std::string, std::string>> &headers();
};


class RequestData {
	public:
		std::string method;
		std::string uri;
		std::string body;
		std::map <std::string, std::string> setCookies;
		std::string ip;

		void setCookiesFromString (const char* cookies_);
};

class Resource {
	private:
		std::string _uri = "";

		static int _staticProcessRequest (mg_connection* conn, void* cbdata);
		std::string _readRequestBody (mg_connection* conn);
		int _processRequest (mg_connection* conn);
		
	public:
		Resource (mg_context *ctx, std::string uri);

		virtual Response processRequest (RequestData &rd);

		std::string_view uri ();
};
