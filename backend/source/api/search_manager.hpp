
#pragma once


#include <memory>
#include <vector>
#include <set>
#include <map>
#include <chrono>

#include "../database.hpp"
#include "resource_upload.hpp"


class SingleSearchResult;
class SearchQuery;
class SearchResults;
class CachedSearch;
class SearchManager;

typedef std::chrono::time_point <std::chrono::system_clock> SearchTimepoint;


class SingleSearchResult {
	private:
		int _resourceId;
		int _refId;
		std::string _url;
		std::string _title;
		std::string _descr;
		std::string _type;
		std::string _pictureUrl;
		float _value;

	public:
		SingleSearchResult (int resource, int refId, std::string url, std::string title, std::string description, std::string type, std::string picUrl, float value);
		SingleSearchResult (const pqxx::row &row, float value = -2);

		int id () const;
		int referencedId () const;
		std::string url () const;
		std::string title () const;
		std::string description () const;
		std::string type () const;
		std::string pictureUrl () const;
		float value () const;

		float addValue (float v);

		std::partial_ordering operator<=> (const SingleSearchResult &r) const;
};

void to_json (nlohmann::json &json, const SingleSearchResult &r);


class SearchQuery {
	private:
		std::string _query;
		std::set <std::string> _allowedTypes;

		std::string _compStr;
		void _generateComparisonString ();
		
	public:
		SearchQuery (std::string query, const std::set <std::string> &allowedTypes);
		SearchQuery (std::string query);

		std::partial_ordering operator<=> (const SearchQuery &q) const;
		bool allowAll () const;
		std::string query () const;

		std::string sqlKeywordConditionQuery (OwnedConnection &conn) const;
		std::string allowedTypesSqlArray () const;
};


class SearchResults {
	private:
		std::vector <SingleSearchResult> _results;

	public:
		SearchResults (std::vector <SingleSearchResult> &&_results);

		int size() const;
		std::vector <SingleSearchResult> slice (int start, int length = 10) const;
};


class SearchResultsBuilder {
	private:
		std::map <int, SingleSearchResult> _results;

	public:
		SearchResultsBuilder ();

		void addResult (SingleSearchResult r);
		bool hasResourceId (int id);
		void addValueToResource (int id, float val);
		SearchResults finalize();

		int size() const;
};


class CachedSearch {
	private:
		SearchResults _results;
		SearchQuery _query;
		SearchTimepoint _validUntil;

	public:
		CachedSearch (SearchResultsBuilder res, SearchQuery q);

		const SearchResults &results ();
		const SearchQuery &query ();
		SearchTimepoint validUntil ();

		static std::chrono::duration <int64_t> cacheDuration ();
};


class SearchManager {
	private:
		static std::unique_ptr <SearchManager> _manager;

		std::map <SearchQuery, CachedSearch> _cache;

		SearchManager ();

	public:
		SearchManager (const SearchManager &) = delete;
		SearchManager (SearchManager &&) = delete;

		static SearchManager &get();

		std::vector <SingleSearchResult> search (SearchQuery query, int pos, int len);
		std::vector <SingleSearchResult> search (std::string query, int pos = 10, int len = 10);
		std::vector <SingleSearchResult> search (std::string query, std::set <std::string> allowedTypes, int pos = 10, int len = 10);

		void clearOldCache ();


		int indexNewResource (int refId, std::string url, std::string title, std::string descr, std::string type);
		void clearIndexForResource (int resourceId);
		
		std::set <std::string> getKeywords (std::string str) const;
		void indexStringForResource (int resourceId, std::string str, int value);
		void setPictureForResource (int resourceId, int pictureResourceId);
};
