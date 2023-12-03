
#include "search_manager.hpp"


SingleSearchResult::SingleSearchResult (int resource, int refId, std::string url, std::string title, std::string description, std::string type, std::string picUrl, float value):
_resourceId (resource),
_refId (refId),
_url (url),
_title (title),
_descr (description),
_type (type),
_pictureUrl (picUrl),
_value (value) {

}

SingleSearchResult::SingleSearchResult (const pqxx::row &row, float value) {
	this->_resourceId = row.at ("id").as <int>();
	this->_refId = row.at ("referenced_id").as <int>();
	this->_url = row.at ("url").as <std::string>();
	this->_title = row.at ("title").as <std::string>();
	this->_descr = row.at ("description").as <std::string>();
	this->_type = row.at ("type").as <std::string>();
	if (value < 0) {
		this->_value = row.at ("value").as <int>();
	} else {
		this->_value = value;
	}

	if (row.at ("picture").is_null() == false) {
		int picResourceId = row.at ("picture").as <int> ();
		auto &mgr = UploadedResourcesManager::get();
		this->_pictureUrl = mgr.getDownloadUrl (picResourceId);
	}
}

int SingleSearchResult::id () const {
	return this->_resourceId;
}

int SingleSearchResult::referencedId () const {
	return this->_refId;
}

std::string SingleSearchResult::url () const {
	return this->_url;
}

std::string SingleSearchResult::title () const {
	return this->_title;
}

std::string SingleSearchResult::description () const {
	return this->_descr;
}

std::string SingleSearchResult::type () const {
	return this->_type;
}

std::string SingleSearchResult::pictureUrl () const {
	return this->_pictureUrl;
}

float SingleSearchResult::value () const {
	return this->_value;
}

float SingleSearchResult::addValue (float v) {
	this->_value += v;
	return this->_value;
}

std::partial_ordering SingleSearchResult::operator<=> (const SingleSearchResult &r) const {
	return this->_value <=> r._value;
}


void to_json (nlohmann::json &j, const SingleSearchResult &r) {
	j ["referenced_id"] = r.referencedId();
	j ["url"] = r.url();
	j ["title"] = r.title();
	j ["description"] = r.description();
	j ["type"] = r.type();
	
	std::string picUrl = r.pictureUrl();
	if (picUrl != "") j ["picture_url"] = picUrl;
}





SearchQuery::SearchQuery (std::string query, const std::set <std::string> &allowedTypes):
_query (query), _allowedTypes (allowedTypes) {
	this->_generateComparisonString();
}

SearchQuery::SearchQuery (std::string query):
_query (query) {
	this->_generateComparisonString();
}

void SearchQuery::_generateComparisonString () {
	this->_compStr = this->_query;
	for (auto &i: this->_allowedTypes) {
		this->_compStr += '\n' + i;
	}
}

std::partial_ordering SearchQuery::operator<=> (const SearchQuery &q) const {
	return this->_compStr.compare (q._compStr) <=> 0;
}

bool SearchQuery::allowAll () const {
	return this->_allowedTypes.size() == 0;
}

std::string SearchQuery::query () const {
	return this->_query;
}


std::string SearchQuery::sqlKeywordConditionQuery (OwnedConnection &conn) const {
	return " search_index.keyword <<% "s + conn.quote (this->_query) + " ";
}

std::string SearchQuery::allowedTypesSqlArray () const {
	std::string res = "(";
	bool nf = false;
	for (auto &i: this->_allowedTypes) {
		if (nf) res += ',';
		res += "'" + i + "'";
		nf = true;
	}
	return res + ")"; // Safe data, no need to escape
}




SearchResults::SearchResults (std::vector <SingleSearchResult> &&res):
_results (res) {

}


int SearchResults::size() const {
	return this->_results.size();
}

std::vector <SingleSearchResult> SearchResults::slice (int first, int length) const {
	if (first > this->size()) return {};
	int last = std::min (this->size() - 1, first + length - 1);
	std::vector <SingleSearchResult> result;
	result.reserve (last - first + 1);

	for (int i = first; i <= last; i++) {
		result.push_back (this->_results [i]);
	}

	return result;
}


SearchResultsBuilder::SearchResultsBuilder () {

}

void SearchResultsBuilder::addResult (SingleSearchResult r) {
	if (this->_results.contains (r.id())) {
		this->_results.at (r.id()).addValue (r.value());
	} else {
		this->_results.insert (std::make_pair (r.id(), r));
	}
}

bool SearchResultsBuilder::hasResourceId (int id) {
	return this->_results.contains (id);
}

void SearchResultsBuilder::addValueToResource (int id, float val) {
	this->_results.at (id).addValue (val);
}

SearchResults SearchResultsBuilder::finalize() {
	std::vector <SingleSearchResult> fin;
	fin.reserve (this->size());

	std::set <SingleSearchResult> toOrder;
	for (auto &i: this->_results) {
		toOrder.insert (i.second);
	}

	for (auto &i: toOrder) {
		fin.push_back (i);
	}

	return SearchResults (std::move (fin));
}

int SearchResultsBuilder::size() const {
	return this->_results.size();
}


CachedSearch::CachedSearch (SearchResultsBuilder res, SearchQuery q):
_results (res.finalize()), _query (q) {
	this->_validUntil = SearchTimepoint::clock::now() + CachedSearch::cacheDuration();
}

const SearchResults &CachedSearch::results () {
	return this->_results;
}

const SearchQuery &CachedSearch::query () {
	return this->_query;
}

SearchTimepoint CachedSearch::validUntil () {
	return this->_validUntil;
}

std::chrono::duration <int64_t> CachedSearch::cacheDuration () {
	return std::chrono::seconds (60);
}



std::unique_ptr <SearchManager> SearchManager::_manager;

SearchManager::SearchManager () {

}

/* static */ SearchManager &SearchManager::get() {
	if (SearchManager::_manager.get() == nullptr)
		SearchManager::_manager = std::unique_ptr <SearchManager> (new SearchManager{});
	return *SearchManager::_manager;
}

std::vector <SingleSearchResult> SearchManager::search (SearchQuery query, int pos, int len) {
	auto now = SearchTimepoint::clock::now();
	if (this->_cache.contains (query)) {
		if (this->_cache.at (query).validUntil() >= now) {
			return this->_cache.at (query).results().slice (pos, len);
		} else {
			this->_cache.erase (query);
		}
	}

	auto conn = database.connect();

	std::string searchQuery =
		"SELECT id, referenced_id, url, title, description, type, picture, value, "
		"strict_word_similarity (keyword, " + conn.quote (query.query()) + ") as sim "
		"FROM search_index "
		"INNER JOIN indexed_resources ON search_index.resource_id=indexed_resources.id "
		"WHERE "s + query.sqlKeywordConditionQuery (conn);
	if (query.allowAll() == false) {
		searchQuery += " AND type IN "s + query.allowedTypesSqlArray() + " ";
	}
	searchQuery += ";";

	auto result = conn.exec (searchQuery);

	SearchResultsBuilder resultBuilder{};

	for (int i = 0; i < result.size(); i++) {
		auto row = result [i];
		int id = row.at ("id").as <int>();
		float value = row.at ("sim").as <float>() * row.at ("value").as <int>();
		if (resultBuilder.hasResourceId (id)) {
			resultBuilder.addValueToResource (id, value);
		} else {
			resultBuilder.addResult (SingleSearchResult {row, value});
		}
 	}

	this->_cache.emplace (std::make_pair (query, CachedSearch (resultBuilder, query)));
	return this->_cache.at (query).results().slice (pos, len);
}

std::vector <SingleSearchResult> SearchManager::search (std::string query, int pos, int len) {
	SearchQuery sQuery (query);
	return this->search (sQuery, pos, len);
}

std::vector <SingleSearchResult> SearchManager::search (std::string query, std::set <std::string> allowedTypes, int pos, int len) {
	SearchQuery sQuery (query, allowedTypes);
	return this->search (sQuery, pos, len);
}

void SearchManager::clearOldCache () {
	auto now = SearchTimepoint::clock::now();
	std::erase_if (this->_cache, [now] (auto &pair) { return pair.second.validUntil() > now; }); // C++20 ftw
}

int SearchManager::indexNewResource (int refId, std::string url, std::string title, std::string descr, std::string type) {
	auto conn = database.connect();
	std::string insertQuery =
		"INSERT INTO indexed_resources (referenced_id, url, title, description, type)"
		"VALUES ("s
		+ std::to_string (refId) + ","
		+ conn.quoteDontEscapeHtml (url) + ","
		+ conn.quote (title) + ","
		+ conn.quote (descr) + ","
		+ conn.quoteDontEscapeHtml (type)
		+ ") RETURNING id;";
	auto row = conn.exec1 (insertQuery);
	return row.at (0).as <int>();
}

std::set <std::string> SearchManager::getKeywords (std::string str) const {
	std::set <std::string> keywords;
	const std::string whitespaces = " \n\t\v";

	int start = str.find_first_not_of (whitespaces);
	while (start != str.npos && start < str.length()) {
		int end = str.find_first_of (whitespaces, start + 1);

		// Should be lowercase() here but it's easier to use sql LOWER() rather than c++ tools
		std::string keyword = str.substr (start, end - start); 
		keywords.insert (keyword);
		
		// std::cout << keyword << std::endl;
		if (end == str.npos) break;
		start = str.find_first_not_of (whitespaces, end + 1);
	}

	return keywords;
}

void SearchManager::indexStringForResource (int resourceId, std::string str, int value) {
	auto conn = database.connect();
	std::string query = 
		"INSERT INTO search_index (resource_id, keyword, value) "
		"VALUES ";
	auto keywords = this->getKeywords (str);
	bool nf = false;
	for (auto &i: keywords) {
		if (nf) query += ",";
		query += "("s + std::to_string (resourceId) + ","
			+ conn.quote (i) + ","
			+ std::to_string (value) + ")";
		nf = true;
	}
	query += " ON CONFLICT (resource_id, keyword) DO UPDATE SET value = search_index.value + EXCLUDED.value;";

	conn.exec (query);
	conn.commit();
}

void SearchManager::clearIndexForResource (int resourceId) {
	std::string query = "DELETE FROM search_index WHERE resource_id=" + std::to_string (resourceId) + ";";
	auto conn = database.connect();
	conn.exec (query);
	conn.commit();
}

void SearchManager::setPictureForResource (int resourceId, int pictureResourceId) {
	std::string query =
		"UPDATE search_index SET picture="s + std::to_string (pictureResourceId)
		+ " WHERE id=" + std::to_string (resourceId) + ";";
	auto conn = database.connect();
	conn.exec (query);
	conn.commit();
}


