
#include "random_search_prompt_resource.hpp"


RandomSearchPromptResource::RandomSearchPromptResource (mg_context* ctx, std::string uri):
ApiResource (ctx, uri), randomGenerator (std::chrono::system_clock::now().time_since_epoch().count()) {

}

struct TemporaryRandomSearchPromptOption {
	std::string text, url;
};

std::unique_ptr<ApiResponse> RandomSearchPromptResource::processRequest(RequestData &rd, nlohmann::json body) {
	if (rd.method != "GET") return std::make_unique<ApiResponse> (405);

	std::string getRandomPageQuery = "SELECT url, title FROM indexed_resources ORDER BY random() LIMIT 1;";

	auto conn = database.connect();
	auto result = conn.exec (getRandomPageQuery);

	if (result.size() > 0) {
		auto row = result[0];
		std::string url = row[0].as <std::string>();
		std::string name = row[1].as <std::string>();
		return std::make_unique<ApiResponse> (nlohmann::json{{"text", name}, {"url", url}}, 200);
	}
	
	return std::make_unique <ApiResponse> (nlohmann::json {{"text", "посмотри исходный код проекта на Github"}, {"url", "https://github.com/f3d0rov/collaboration"}});
}
