// TODO: this but better

#pragma once

#include <set>
#include <string>
#include <filesystem>

#include "../api_resource.hpp"
#include "user_auth.hpp"
#include "search_manager.hpp"
#include "resource_upload.hpp"


class EntityData;
class CreatePageResource;
class RequestPictureChangeResource;
class UploadPictureResource;
class EntityDataResource;


class EntityData {
	public:
		bool valid = false;

		int id;
		std::string name;
		std::string description;
		std::string type;

		std::string startDate;
		std::optional <std::string> endDate;

		bool created = false;

		std::optional <int> createdBy;
		std::optional <std::string> createdOn;

		std::optional <std::string> picturePath;
};

bool entityIsIndexed (int entityId);
int getEntitySearchIndexResource (int entityId);
int indexEntity (int entityId, EntityData &data);
int clearEntityIndex (int entityId, EntityData &data);
int updateEntityIndex (int entityId);

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
		int createEntity (OwnedConnection &work, std::string type, std::string name, std::string desc, std::string startDate, std::string endDate, int uid);
		static int createEmptyEntityWithWork (OwnedConnection &work, const std::string &name);
		int createTypedEntity (OwnedConnection &work, int entityId, std::string type);

		static std::string pageUrlForTypedEntity (std::string type, int id);
		static std::string urlForId (int id);
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


class EntityDataResource: public ApiResource {
	private:
		std::string _pics;
	public:
		EntityDataResource (mg_context *ctx, std::string uri, std::string picsUri);
		static bool entityCreated (int id);
		static int getEntityByNameWithWork (OwnedConnection &work, const std::string &name);
		static int getEntityByName (const std::string &name);
		static EntityData getEntityDataByIdWithWork (OwnedConnection &work, int id);
		static EntityData getEntityDataById (int id);
		std::unique_ptr <ApiResponse> processRequest (RequestData &rd, nlohmann::json body) override;
};
