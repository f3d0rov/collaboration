
#include "search.hpp"


SearchResult::SearchResult () {

}

SearchResult::SearchResult (std::string type, std::string text, std::string url):
type (type), text (text), url (url) {

}

SearchResult::SearchResult (std::string type, std::string text, std::string url, std::string imgPath):
type (type), text (text), url (url), imgPath (imgPath) {
	
}

auto SearchResult::operator <=> (const SearchResult &right) const {
	return this->value <=> right.value;
}

void to_json(nlohmann::json& j, const SearchResult& sr) {
	j = nlohmann::json{{"text", sr.text}, {"url", sr.url}, {"type", sr.type}};
}




SearchResource::SearchResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

std::set <std::string> SearchResource::getKeywordsFromPrompt (std::string prompt) {
	std::set <std::string> result;
	const std::string whitespaces = " \n\t\v";

	int start = prompt.find_first_not_of (whitespaces);
	while (start != prompt.npos && start < prompt.length()) {
		int end = prompt.find_first_of (whitespaces, start + 1);
		std::string keyword = lowercase (prompt.substr (start, end - start));
		result.insert (keyword);
		// std::cout << keyword << std::endl;
		if (end == prompt.npos) break;
		start = prompt.find_first_not_of (whitespaces, end + 1);
	}

	return result;
}

std::string SearchResource::getSqlListOfKeywords (std::set <std::string> &keywords, pqxx::work &work) {
	std::string result = "(";

	bool notFirst = false;
	for (auto i: keywords) {
		if (notFirst) result += ",";
		result += work.quote (i);
		notFirst = true;
	}

	return result + ")";
}

std::vector <SearchResult> SearchResource::findAllWithWork (std::string prompt, pqxx::work &work) {
	auto keywords = SearchResource::getKeywordsFromPrompt (prompt);
	if (keywords.size() == 0) return {};
	std::string keywordList = SearchResource::getSqlListOfKeywords (keywords, work);
	std::string searchQuery = std::string ("SELECT url, title, value, type FROM search_index WHERE keyword IN ") + keywordList + ";"; 

	auto result = work.exec (searchQuery);
	if (result.size() == 0) return {};

	std::map <std::string /* url */, SearchResult> urlList;
	for (int i = 0; i < result.size(); i++) {
		auto row = result[i];
		std::string url = row[0].as <std::string>();
		std::string title = row[1].as <std::string>();
		int value = row[2].as <int>();
		std::string type = row[3].as <std::string>();

		if (!urlList.contains (url)) {
			urlList [url] = SearchResult (type, title, url);
		}
		urlList [url].value += value;
	}

	std::set <SearchResult> priorityList;
	for (const auto &i: urlList) {
		priorityList.insert (i.second);
	}

	return std::vector <SearchResult> (priorityList.begin(), priorityList.end());
}

std::vector <SearchResult> SearchResource::findAll (std::string prompt) {
	auto conn = database.connect();
	pqxx::work work (*conn.conn);
	auto result = SearchResource::findAllWithWork (prompt, work);
	work.commit ();
	return result;
}

std::vector <SearchResult> SearchResource::findByTypeWithWork (std::string prompt, std::string type, pqxx::work &work) {
	return {};
}

std::vector <SearchResult> SearchResource::findByType (std::string prompt, std::string type) {
	auto conn = database.connect();
	pqxx::work work (*conn.conn);
	auto result = SearchResource::findByTypeWithWork (prompt, type, work);
	work.commit ();
	return result;
}

std::unique_ptr<ApiResponse> SearchResource::processRequest (RequestData &rd, nlohmann::json body) {
	if (rd.method != "POST") return std::make_unique <ApiResponse> (405);
	if (!body.contains ("prompt")) return std::make_unique <ApiResponse> (401);
	std::string prompt;

	try {
		prompt = body ["prompt"].get <std::string> ();
	} catch (nlohmann::json::exception &e) {
		return std::make_unique <ApiResponse> (nlohmann::json {{"error", e.what()}}, 401);
	}

	auto result = SearchResource::findAll (prompt);
	return std::make_unique <ApiResponse> (nlohmann::json{{"results", result}}, 200);
}
