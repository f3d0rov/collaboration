#pragma once

#include <vector>
#include <memory>
#include <fstream>
#include <filesystem>
#include <exception>
#include <optional>
#include <sstream>
#include <chrono>

#include "civetweb/CivetServer.h"

#include "resource.hpp"
#include "utils.hpp"


#define CACHED_FILES_RELOAD_PERIOD (std::chrono::minutes (5))


class WebResource;
class SharedDirectory;


class WebResource: public Resource {
	private:
		std::string _filepath, _mime;
		std::optional <std::string> _cached;
		std::chrono::time_point <std::chrono::system_clock> _cacheTime;

		void _cacheFile ();
		bool _cacheIsInvalid ();
		void _loadCacheIfInvalid ();

	protected:
		std::string _getActualCache ();

	public:
		WebResource (mg_context *ctx, std::string uri, std::string path, std::string mime = "text/html");
		virtual Response processRequest (std::string method, std::string uri, std::string body) override;

		std::string filepath();
		std::string mime();
};


class SharedDirectory {
	private:
		std::vector < std::unique_ptr <WebResource> > _resources;

	public:
		SharedDirectory (mg_context *ctx, std::string directoryPath, bool ignoreHtml = true);

		size_t size();
};
 