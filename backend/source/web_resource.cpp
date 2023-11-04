
#include "web_resource.hpp"


WebResource::WebResource (mg_context *ctx, std::string uri, std::string path, bool dontCache, std::string mime):
Resource (ctx, uri) {
	this->_filepath = path;
	this->_mime = mime;
	this->_dontCache = dontCache;

	logger << "Ресурс: \"/" << uri << "\" (" << path << ") [" << std::filesystem::file_size (path) << "B, " << mime << "]" <<  std::endl;
}

void WebResource::_cacheFile () {
	this->_cached.emplace (readFile (this->_filepath, std::ios::binary));
	this->_cacheTime = std::chrono::system_clock::now();
}

bool WebResource::_cacheIsInvalid () {
	return ( // Reload cache if
		this->_dontCache 				// Always reloading file
		|| (!this->_cached.has_value()) // Didn't cache before
		|| ((this->_cacheTime + CACHED_FILES_RELOAD_PERIOD) <= std::chrono::system_clock::now()) // Cache too old
	);
}

void WebResource::_loadCacheIfInvalid () {
	if (this->_cacheIsInvalid()) {
		this->_cacheFile();
	}
}

std::string WebResource::_getActualCache () {
	this->_loadCacheIfInvalid ();
	return this->_cached.value();
}

std::unique_ptr<_Response> WebResource::processRequest (RequestData &rd) {
	return std::make_unique<Response> (this->_getActualCache(), this->_mime);
}

std::string WebResource::filepath () {
	return this->_filepath;
}

std::string WebResource::mime () {
	return this->_mime;
}



SharedDirectory::SharedDirectory (mg_context *ctx, std::string directoryPath, bool ignoreHtml, bool dontCache) {
	for (auto& i: std::filesystem::recursive_directory_iterator (directoryPath)) {
		auto path = i.path();
		if (!std::filesystem::is_regular_file (path)) continue;

		auto relPath = std::filesystem::relative (path, directoryPath);
		std::string fullPathStr = path.string();
		std::string uri = relPath.string();
		std::string ext = getFileExtension (uri);

		if (ignoreHtml && (ext == "html" || ext == "htm")) continue;
		std::string mime = mg_get_builtin_mime_type (uri.c_str());

		this->_resources.emplace_back (std::make_unique <WebResource> (ctx, uri, fullPathStr, dontCache, mime));
	}
}

size_t SharedDirectory::size() {
	return this->_resources.size();
}


DynamicDirectory::DynamicDirectory (mg_context *ctx, std::string uri, std::filesystem::path dir):
Resource (ctx, uri), _dir (dir) {

}

std::filesystem::path DynamicDirectory::pathFromUri (std::string uri) {
	int start = uri.find (this->uri());
	std::string path = uri.substr (start + this->uri().length());
	if (path.length() == 0) return this->_dir; // will return 404 because of is_regular_file check
	if (path[0] == '/') path = path.substr (1);
	return this->_dir / std::filesystem::path (path);
}

std::unique_ptr <_Response> DynamicDirectory::processRequest (RequestData &rd) {
	// Civetweb resolves .. paths so no need to worry about accidentally exposing other files
	std::filesystem::path path = this->pathFromUri (rd.uri);
	logger << path << std::endl;
	if (!(std::filesystem::exists (path) && std::filesystem::is_regular_file (path)))
		return std::make_unique <Response> ("not found", 404);
	logger << "Serving " << path << std::endl;
	return std::make_unique <Response> (readFile (path, std::ios::binary), mg_get_builtin_mime_type (path.c_str()), 200);
}
