
#include "random_search_prompt_resource.hpp"


RandomSearchPromptResource::RandomSearchPromptResource (mg_context* ctx, std::string uri):
ApiResource (ctx, uri), randomGenerator (std::chrono::system_clock::now().time_since_epoch().count()) {

}

struct TemporaryRandomSearchPromptOption {
	std::string text, url;
};

std::unique_ptr<ApiResponse> RandomSearchPromptResource::processRequest(RequestData &rd, nlohmann::json body) {
	if (rd.method != "GET") return std::make_unique<ApiResponse> (405);

	std::string getRandomPageQuery = "SELECT id, name FROM entities WHERE awaits_creation=FALSE ORDER BY random() LIMIT 1;";

	auto conn = database.connect();
	auto result = conn.exec (getRandomPageQuery);

	if (result.size() > 0) {
		auto row = result[0];
		int id = row[0].as <int>();
		std::string name = row[1].as <std::string>();
		std::string url = "";

		url = CreatePageResource::pageUrlForTypedEntity ("", id);

		return std::make_unique<ApiResponse> (nlohmann::json{{"text", name}, {"url", url}, {"id", id}}, 200);
	}
	
	return std::make_unique <ApiResponse> (nlohmann::json {{"text", "посмотри исходный код проекта на Github"}, {"url", "https://github.com/f3d0rov/collaboration"}});
}
