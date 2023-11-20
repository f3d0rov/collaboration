
#include "randomSearchPromptResource.hpp"


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
	pqxx::work work (*conn.conn);
	auto result = work.exec (getRandomPageQuery);

	if (result.size() > 0) {
		auto row = result[0];
		int id = row[0].as <int>();
		std::string type = row[2].as <std::string>();
		std::string url = "";

		url = CreatePageResource::pageUrlForTypedEntity (type, id);

		return std::make_unique<ApiResponse> (nlohmann::json{{"text", result[0][1].c_str()}, {"url", url}, {"id", id}}, 200);
	}
	
	return std::make_unique <ApiResponse> (nlohmann::json {{"text", "посмотри исходный код проекта на Github"}, {"url", "https://github.com/f3d0rov/collaboration"}});
}
