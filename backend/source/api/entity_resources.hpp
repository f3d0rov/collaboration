
#pragma once

#include <set>
#include <string>
#include <filesystem>

#include "../api_resource.hpp"
#include "user_auth.hpp"
#include "search_manager.hpp"
#include "resource_upload.hpp"

#include "entity_manager.hpp"


class GetEntityTypesResource;
class CreateEntityResource;
class GetEntityResource;
class RequestEntityPictureChangeResource;




class GetEntityTypesResource: public ApiResource {
	public:
		GetEntityTypesResource (mg_context *ctx, std::string uri);
		ApiResponsePtr processRequest (RequestData &rd, nlohmann::json body) override;
};


/***********
 * POST { 
 * 		"type": "person|band|album", 
 * 		"name": name, 
 * 		"description": decription,
 * 		"start_date": birth/establishment/publication date, 
 * 		"end_date": day of death (may be missing)
 * }
*/
class CreateEntityResource: public ApiResource {
	public:
		CreateEntityResource (mg_context *ctx, std::string uri);
		ApiResponsePtr processRequest (RequestData &rd, nlohmann::json body) override;
};


class GetEntityResource: public ApiResource {
	public:
		GetEntityResource (mg_context *ctx, std::string uri);
		ApiResponsePtr processRequest (RequestData &rd, nlohmann::json body) override;
};

/************
 * POST {
 * 		"entity_id": id
 * } -> {
 * 		"url": "/uploadpic?id=o21uroiwhjfsdjofpgsfg..."
 * }
*/
class RequestEntityPictureChangeResource: public ApiResource {
	public:
		RequestEntityPictureChangeResource (mg_context *ctx, std::string uri);
		ApiResponsePtr processRequest (RequestData &rd, nlohmann::json body) override;
};
