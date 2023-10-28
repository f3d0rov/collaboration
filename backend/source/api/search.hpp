#pragma once

#include <vector>
#include <map>
#include <string>
#include <set>

#include "../api_resource.hpp"
#include "../database.hpp"


class SearchResult;
class SearchResource;


class SearchResult {
	public:
		std::string text;
		std::string url;
		std::optional <std::string> imgPath;

		int value = 0;
		std::string type = "none";

		SearchResult ();
		SearchResult (std::string type, std::string text, std::string url = "");
		SearchResult (std::string type, std::string text, std::string url, std::string imgPath);

		auto operator <=> (const SearchResult &right) const;
		operator nlohmann::json () const;
};


// POST {"prompt": str-prompt} -> {"results": [ {"text": text, "url": url, "type": type }, {}]}
class SearchResource: public ApiResource {
	private:
		static std::set <std::string> getKeywordsFromPrompt (std::string prompt);
		static std::string getSqlListOfKeywords (std::set <std::string> &keywords, pqxx::work &work);
		
	public:
		SearchResource (mg_context *ctx, std::string uri);
		static std::vector <SearchResult> findAll (std::string prompt);
		static std::vector <SearchResult> findAllWithWork (std::string prompt, pqxx::work &work);
		static std::vector <SearchResult> findByTypeWithWork (std::string prompt, std::string type, pqxx::work &work);
		static std::vector <SearchResult> findByType (std::string prompt, std::string type);

		static nlohmann::json sliceOfSearchResultVector (const std::vector <SearchResult> &vec, int start, int end);
		std::unique_ptr<ApiResponse> processRequest (RequestData &rd, nlohmann::json body) override;
};
