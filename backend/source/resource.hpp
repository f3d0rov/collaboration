#pragma once

#include <memory>
#include <string>

#include "civetweb/CivetServer.h"

#include "utils.hpp"
#include "api/user_manager.hpp" // .updateUserLastActionTime ();, SESSION_ID

#define BUFFER_SIZE (16 * 1024) /* 16 Kb */
#define MAX_BODY_SIZE (16 * 1024 * 1024) /* 16 Mb */

class _Response;
class Response;
class Resource;

class _Response {
	private:
		std::vector <std::pair<std::string, std::string>> _customHeaders;
	public:
		int status = 200;

		_Response ();
		virtual ~_Response ();

		virtual std::string getBody () = 0;
		virtual std::string getMime () = 0;
		
		_Response& addHeader (std::string name, std::string value);
		_Response& setCookie (std::string name, std::string value, bool httpOnly = false, long long maxAge = 60 * 60 * 24 * 7);
		const std::vector <std::pair<std::string, std::string>> &headers();
		
		void redirect (std::string address);
};

class Response: public _Response {
	public:
		std::string body = "";
		std::string mime = "text/html";

		Response (int status);
		Response (std::string body, int status = 200);
		Response (std::string body, std::string mimeType, int status = 200);
		~Response ();

		std::string getBody () final;
		std::string getMime () final;
};


class RequestData {
	public:
		std::string method;
		std::string uri;
		std::string body;
		std::map <std::string, std::string> setCookies;
		std::map <std::string, std::string> query;
		std::string ip;

		void setCookiesFromString (const char* cookies_);
		void setQueryVariables (const char *query);

		std::string getCookie (std::string name, std::string defaultValue = "");
};


class Resource {
	private:
		std::string _uri = "";

		static int _staticProcessRequest (mg_connection* conn, void* cbdata);
		std::string _readRequestBody (mg_connection* conn);
		int _processRequest (mg_connection* conn);
		
	public:
		Resource (mg_context *ctx, std::string uri);
		virtual ~Resource();

		virtual std::unique_ptr<_Response> processRequest (RequestData &rd);

		std::string_view uri ();
};
