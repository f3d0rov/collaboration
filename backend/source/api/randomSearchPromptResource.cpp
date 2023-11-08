
#include "randomSearchPromptResource.hpp"


RandomSearchPromptResource::RandomSearchPromptResource (mg_context* ctx, std::string uri):
ApiResource (ctx, uri), randomGenerator (std::chrono::system_clock::now().time_since_epoch().count()) {

}

struct TemporaryRandomSearchPromptOption {
	std::string text, url;
};

std::unique_ptr<ApiResponse> RandomSearchPromptResource::processRequest(RequestData &rd, nlohmann::json body) {
	if (rd.method != "GET") return std::make_unique<ApiResponse> (405);

	std::string getRandomPageQuery = "SELECT id, name, type FROM entities WHERE awaits_creation=FALSE ORDER BY random() LIMIT 1;";

	auto conn = database.connect();
	pqxx::work work (*conn.conn);
	auto result = work.exec (getRandomPageQuery);

	if (result.size() > 0) {
		int id = result[0][0].as <int>();
		std::string type = result[0][2].as <std::string>();
		std::string url = "";
		
		const std::map <std::string, std::string> tables = {
			{"person", "personalities"},
			{"band", "bands"},
			{"album", "albums"}
		};

		std::string getTypedIdQuery = "SELECT id FROM " + tables.at (type) + " WHERE entity_id = " + result[0][0].c_str() + ";";
		pqxx::row getTypedIdRow = work.exec1 (getTypedIdQuery);
		int tid = getTypedIdRow[0].as <int>();

		url = CreatePageResource::pageUrlForTypedEntity (type, tid);

		return std::make_unique<ApiResponse> (nlohmann::json{{"text", result[0][1].c_str()}, {"url", url}}, 200);
	}
	
	return std::make_unique <ApiResponse> (nlohmann::json {{"text", "посмотри исходный код проекта на Github"}, {"url", "https://github.com/f3d0rov/collaboration"}});
}
