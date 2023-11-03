#pragma once

#include <set>
#include <string>

#include "../api_resource.hpp"
#include "user_auth.hpp"
#include "search.hpp"


/***********
 * POST { 
 * 		"type": "person|band|album", 
 * 		"name": name, 
 * 		"description": decription,
 * 		"start_date": birth/establishment/publication date, 
 * 		"end_date": day of death (may be missing)
 * }
*/
class CreatePageResource: public ApiResource {
	public:
		CreatePageResource (mg_context *ctx, std::string uri);
		int createEntity (pqxx::work &work, std::string type, std::string name, std::string desc, std::string startDate, std::string endDate, int uid);
		int createTypedEntity (pqxx::work &work, int entityId, std::string type);

		static std::string pageUrlForTypedEntity (std::string type, int id);
		std::unique_ptr <ApiResponse> processRequest (RequestData &rd, nlohmann::json body) override;
};

#if 0

class UploadPictureResource: public Resource {
	public:
		
};

#endif

class EntityDataResource: public ApiResource {
	private:
		std::string _table;
	public:
		EntityDataResource (mg_context *ctx, std::string uri, std::string entityTable);
		std::unique_ptr <ApiResponse> processRequest (RequestData &rd, nlohmann::json body) override;
};
