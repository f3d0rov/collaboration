
#include "search.hpp"


SearchResult::SearchResult () {

}

SearchResult::SearchResult (std::string type, std::string title, std::string url):
type (type), title (title), url (url) {

}

SearchResult::SearchResult (std::string type, std::string title, std::string url, std::string imgPath):
type (type), title (title), url (url), imgPath (imgPath) {
	
}

// SQL row constructor, __specifically__ ordered as 'url, title, type, picture_path, value, desc'. `picture_path` may be NULL.
SearchResult::SearchResult (const pqxx::row& sqlrow) {
	// This could be implemented without the need for strict order,
	// but pqxx documentation says that access by column name is slower 
	// than access by index. Considering that during one search this constructor
	// might be called dozens of times, optimization is best.
	this->url = sqlrow[0].as <std::string>();
	this->title = sqlrow[1].as <std::string>();
	this->type = sqlrow[2].as <std::string>();
	this->value = sqlrow[4].as <int>();
	if (!sqlrow[3].is_null()) this->imgPath = sqlrow[3].as <std::string>();
	if (!sqlrow[5].is_null()) this->text = sqlrow[5].as <std::string>();
}

auto SearchResult::operator <=> (const SearchResult &right) const {
	return this->value <=> right.value;
}

void to_json(nlohmann::json& j, const SearchResult& sr) {
	j = nlohmann::json{{"title", sr.title}, {"url", sr.url}, {"type", sr.type}};
	if (sr.imgPath.has_value()) j["picture_path"] = *sr.imgPath;
	if (sr.text.has_value()) j["text"] = *sr.text;
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

		// Should be lowercase() here but it's easier to use sql LOWER() rather than c++ tools
		std::string keyword = prompt.substr (start, end - start); 
		result.insert (keyword);
		
		// std::cout << keyword << std::endl;
		if (end == prompt.npos) break;
		start = prompt.find_first_not_of (whitespaces, end + 1);
	}

	return result;
}

std::string SearchResource::getSqlListOfKeywords (std::set <std::string> &keywords, pqxx::work &work) {
	std::string result;

	bool notFirst = false;
	for (auto i: keywords) {
		if (notFirst) result += "|";
		result += i;
		notFirst = true;
	}

	return work.quote(result);
}

std::vector <SearchResult> SearchResource::findAllWithWork (std::string prompt, pqxx::work &work) {
	auto keywords = SearchResource::getKeywordsFromPrompt (prompt);
	if (keywords.size() == 0) return {};
	std::string keywordList = SearchResource::getSqlListOfKeywords (keywords, work);
	
	std::string searchQuery = std::string ()
		+ "SELECT url, title, type, picture_path, SUM(value), description "
		+ "FROM indexed_resources INNER JOIN search_index ON indexed_resources.id = search_index.resource_id "
		+ "WHERE keyword ~* " + keywordList + " GROUP BY url, title, type, picture_path, description ORDER BY SUM(value) DESC;";

	logger << searchQuery << std::endl;
	auto result = work.exec (searchQuery);
	if (result.size() == 0) return {};

	std::vector <SearchResult> results;
	for (int i = 0; i < result.size(); i++) {
		results.emplace_back (result[i]);
	}

	return results;
}

std::vector <SearchResult> SearchResource::findAll (std::string prompt) {
	auto conn = database.connect();
	pqxx::work work (*conn.conn);
	auto result = SearchResource::findAllWithWork (prompt, work);
	work.commit ();
	return result;
}

std::vector <SearchResult> SearchResource::findByTypeWithWork (std::string prompt, std::string type, pqxx::work &work) {

	auto keywords = SearchResource::getKeywordsFromPrompt (prompt);
	if (keywords.size() == 0) return {};
	std::string keywordList = SearchResource::getSqlListOfKeywords (keywords, work);
	
	std::string searchQuery = std::string ()
		+ "SELECT url, title, type, picture_path, SUM(value), description "
		+ "FROM indexed_resources INNER JOIN search_index ON indexed_resources.id = search_index.resource_id "
		+ "WHERE keyword ~* " + keywordList + " AND type=" + work.quote (type)
		+" GROUP BY url, title, type, picture_path, description ORDER BY SUM(value) DESC;";

	auto result = work.exec (searchQuery);
	if (result.size() == 0) return {};

	std::vector <SearchResult> results;
	for (int i = 0; i < result.size(); i++) {
		results.emplace_back (result[i]);
	}

	return results;
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

	auto start = std::chrono::high_resolution_clock::now();
	auto result = SearchResource::findAll (prompt);
	auto timeToExec = usElapsedFrom_hiRes (start);
	return std::make_unique <ApiResponse> (nlohmann::json{{"results", result}, {"time", prettyMicroseconds (timeToExec)}}, 200);
}
