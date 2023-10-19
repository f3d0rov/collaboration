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
		int status = 200;

		Response (int status);
		Response (std::string body, int status = 200);
};


class Resource {
	private:
		std::string _uri = "";

		static int _staticProcessRequest (mg_connection* conn, void* cbdata);
		std::string _readRequestBody (mg_connection* conn);
		int _processRequest (mg_connection* conn);
	public:
		Resource (mg_context *ctx, std::string uri);

		virtual Response processRequest (std::string method, std::string uri, std::string body);

		std::string_view uri ();
};
