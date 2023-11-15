#pragma once

#include <vector>
#include <map>
#include <string>
#include <set>

#include "../api_resource.hpp"
#include "../database.hpp"


class PrimitiveSearchResult;
class SearchResult;
class PromptToken;
class Searcher;
class SearchResource;
class EntityTypeSearchResource;


class PrimitiveSearchResult {
	public:
		std::string title, type;
		int id;

		
		PrimitiveSearchResult (std::string title, std::string type, int id);
};

void to_json(nlohmann::json& j, const PrimitiveSearchResult& sr);


class SearchResult {
	public:
		std::string title;
		std::string url;
		std::optional <std::string> imgPath;
		std::optional <std::string> text;

		int value = 0;
		std::string type = "none";

		SearchResult ();
		SearchResult (std::string type, std::string title, std::string url = "");
		SearchResult (std::string type, std::string title, std::string url, std::string imgPath);
		
		// SQL row constructor, __specifically__ ordered as 'url, title, type, picture_path, value'
		SearchResult (const pqxx::row& sqlrow);

		auto operator <=> (const SearchResult &right) const;
		operator nlohmann::json () const;
};


class PromptToken {
	public:
		std::string keyword;
		int value;

		PromptToken (std::string keyword);
};


class Searcher {
	private:
		static std::set <std::string> getKeywordsFromPrompt (std::string prompt);
		static std::string getSqlListOfKeywords (std::set <std::string> &keywords, pqxx::work &work);
		static std::string getTableForType (std::string type);
	public:
		static std::vector <SearchResult> findAll (std::string prompt);
		static std::vector <SearchResult> findAllWithWork (std::string prompt, pqxx::work &work);

		static std::vector <PrimitiveSearchResult> findEntities (std::string prompt);
		static std::vector <PrimitiveSearchResult> findEntitiesWithWork (std::string prompt, pqxx::work &work);

		static std::vector <PrimitiveSearchResult> findByTypeWithWork (std::string prompt, std::string type, pqxx::work &work);
		static std::vector <PrimitiveSearchResult> findByType (std::string prompt, std::string type);

		static std::map <std::string, PromptToken> analysePrompt (std::string prompt);
		static void indexWithWork (pqxx::work &work, std::string type, std::string url, std::string prompt, std::string name, std::string desc, std::string imgPath);
		
		static nlohmann::json sliceOfSearchResultVector (const std::vector <SearchResult> &vec, int start, int end);

};

// POST {"prompt": str-prompt} -> {"results": [ {"text": text, "url": url, "type": type }, {}]}
class SearchResource: public ApiResource {
	public:
		SearchResource (mg_context *ctx, std::string uri);
		std::unique_ptr<ApiResponse> processRequest (RequestData &rd, nlohmann::json body) override;
};


class EntitySearchResource: public ApiResource {
	public:
		EntitySearchResource (mg_context *ctx, std::string uri);
		ApiResponsePtr processRequest (RequestData &rd, nlohmann::json body) override;
};


class TypedSearchResource: public ApiResource {
	private:
		std::string _type;
	public:
		TypedSearchResource (mg_context *ctx, std::string uri, std::string type);
		ApiResponsePtr processRequest (RequestData &rd, nlohmann::json body) override;
};

