
#include "search_resource.hpp"



SearchResource::SearchResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

std::unique_ptr<ApiResponse> SearchResource::processRequest (RequestData &rd, nlohmann::json body) {
	assertMethod (rd, "POST");
	
	std::string prompt = getParameter <std::string> ("prompt", body);
	int page = 0;
	if (body.contains ("page")) page = getParameter <int> ("page", body);

	auto start = std::chrono::high_resolution_clock::now();
	auto &mgr = SearchManager::get();
	auto result = mgr.search (prompt, page, 20);
	auto timeToExec = usElapsedFrom_hiRes (start);
	return std::make_unique <ApiResponse> (nlohmann::json{{"results", result.slice}, {"time", prettyMicroseconds (timeToExec)}, {"total", result.total}}, 200);
}




EntitySearchResource::EntitySearchResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

ApiResponsePtr EntitySearchResource::processRequest (RequestData &rd, nlohmann::json body) {
	assertMethod (rd, "POST");
	
	std::string prompt = getParameter <std::string> ("prompt", body);
	int page = 0;
	if (body.contains ("page")) page = getParameter <int> ("page", body);

	auto start = std::chrono::high_resolution_clock::now();
	auto &mgr = SearchManager::get();
	auto result = mgr.search (prompt, {"person", "band"}, page, 10);
	auto timeToExec = usElapsedFrom_hiRes (start);
	return makeApiResponse (nlohmann::json{{"results", result.slice}, {"time", prettyMicroseconds (timeToExec)}, {"total", result.total}}, 200);
}




TypedSearchResource::TypedSearchResource (mg_context *ctx, std::string uri, std::string type):
ApiResource (ctx, uri), _type (type) {

}

ApiResponsePtr TypedSearchResource::processRequest (RequestData &rd, nlohmann::json body) {
	assertMethod (rd, "POST");
	
	std::string prompt = getParameter <std::string> ("prompt", body);
	int page = 0;
	if (body.contains ("page")) page = getParameter <int> ("page", body);

	auto start = std::chrono::high_resolution_clock::now();
	auto &mgr = SearchManager::get();
	auto result = mgr.search (prompt, {this->_type}, page, 10);
	auto timeToExec = usElapsedFrom_hiRes (start);
	return makeApiResponse (nlohmann::json{{"results", result.slice}, {"time", prettyMicroseconds (timeToExec)}, {"total", result.total}}, 200);
}


