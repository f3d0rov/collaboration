#pragma once

#include <vector>
#include <map>
#include <string>
#include <set>

#include "../api_resource.hpp"
#include "../database.hpp"

#include "search_manager.hpp"


class SearchResource;
class EntitySearchResource;
class TypedSearchResource;


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

