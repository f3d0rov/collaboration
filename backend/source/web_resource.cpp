
#include "web_resource.hpp"


WebResource::WebResource (mg_context *ctx, std::string uri, std::string path, std::string mime):
Resource (ctx, uri) {
	this->_filepath = path;
	this->_mime = mime;

	logger << "Ресурс: \"/" << uri << "\" (" << path << ") [" << std::filesystem::file_size (path) << "B, " << mime << "]" <<  std::endl;
}

void WebResource::_cacheFile () {
	this->_cached.emplace (readFile (this->_filepath, std::ios::binary));
	this->_cacheTime = std::chrono::system_clock::now();
}

bool WebResource::_cacheIsInvalid () {
	return (!this->_cached.has_value()) || (this->_cacheTime + CACHED_FILES_RELOAD_PERIOD <= std::chrono::system_clock::now());
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

Response WebResource::processRequest (RequestData &rd) {
	return Response (this->_getActualCache(), this->_mime);
}

std::string WebResource::filepath () {
	return this->_filepath;
}

std::string WebResource::mime () {
	return this->_mime;
}



SharedDirectory::SharedDirectory (mg_context *ctx, std::string directoryPath, bool ignoreHtml) {
	for (auto& i: std::filesystem::recursive_directory_iterator (directoryPath)) {
		auto path = i.path();
		if (!std::filesystem::is_regular_file (path)) continue;

		auto relPath = std::filesystem::relative (path, directoryPath);
		std::string fullPathStr = path.string();
		std::string uri = relPath.string();
		std::string ext = getFileExtension (uri);

		if (ignoreHtml && (ext == "html" || ext == "htm")) continue;
		std::string mime = mg_get_builtin_mime_type (uri.c_str());

		this->_resources.emplace_back (std::make_unique <WebResource> (ctx, uri, fullPathStr, mime));
	}
}

size_t SharedDirectory::size() {
	return this->_resources.size();
}
