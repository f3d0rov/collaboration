#pragma once

#include <set>
#include <string>
#include <filesystem>

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


/************
 * POST {
 * 		"entity_id": id
 * } -> {
 * 		"url": "/uploadpic?id=o21uroiwhjfsdjofpgsfg..."
 * }
*/
class RequestPictureChangeResource: public ApiResource {
	private:
		std::string _uploadUrl;
	public:
		RequestPictureChangeResource (mg_context *ctx, std::string uri, std::string uploadUrl);
		std::unique_ptr <ApiResponse> processRequest (RequestData &rd, nlohmann::json body) override;
};


/***********
 * PUT @ /uploadpic?id=aFKJBSDFJJ124asdf...
 * data:image/png;base64,JASFLjfnkalff....
*/
class UploadPictureResource: public Resource {
	private:
		std::filesystem::path _savePath;
	public:
		UploadPictureResource (mg_context *ctx, std::string uri, std::filesystem::path savePath);

		void setEntityPicture (pqxx::work &work, int entityId, std::string path);
		std::unique_ptr <_Response> processRequest (RequestData &rd) override;
};


class EntityDataResource: public ApiResource {
	private:
		std::string _table;
		std::string _pics;
	public:
		EntityDataResource (mg_context *ctx, std::string uri, std::string entityTable, std::string picsUri);
		static bool entityCreated (int id);
		std::unique_ptr <ApiResponse> processRequest (RequestData &rd, nlohmann::json body) override;
};
