
#include "page.hpp"

bool entityIsIndexed (int entityId) {
	std::string getIndexQuery =
		"SELECT search_resource "
		"FROM entities "
		"WHERE id="s + std::to_string (entityId) + ";";

	auto conn = database.connect();
	auto row = conn.exec1 (getIndexQuery);
	return row.at (0).is_null() == false;
}

int getEntitySearchIndexResource (int entityId) {
	std::string getIndexQuery =
		"SELECT search_resource "
		"FROM entities "
		"WHERE id="s + std::to_string (entityId) + ";";

	auto conn = database.connect();
	auto row = conn.exec1 (getIndexQuery);
	return row.at (0).as <int> ();
}

int indexEntity (int entityId, EntityData &data) {
	auto &mgr = SearchManager::get();
	int index = mgr.indexNewResource (
		entityId,
		CreatePageResource::urlForId (entityId),
		data.name,
		data.description,
		data.type
	);
	return index;
}

int clearEntityIndex (int entityId, EntityData &data) {
	auto &mgr = SearchManager::get();
	int index = getEntitySearchIndexResource (entityId);
	mgr.updateResourceData (index, data.name, data.description);
	mgr.clearIndexForResource (index);
	return index;
}

int updateEntityIndex (int entityId) {
	auto entityData = EntityDataResource::getEntityDataById (entityId);
	int index;

	if (entityIsIndexed (entityId)) {
		index = clearEntityIndex (entityId, entityData);
	} else {
		index = indexEntity (entityId, entityData);
	}

	auto &mgr = SearchManager::get();

	mgr.indexStringForResource (index, entityData.name, SEARCH_VALUE_TITLE);
	mgr.indexStringForResource (index, entityData.description, SEARCH_VALUE_DESCRIPTION);

	return index;
}


CreatePageResource::CreatePageResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

int CreatePageResource::createEntity (OwnedConnection &work, std::string type, std::string name, std::string desc, std::string startDate, std::string endDate, int uid) {
	std::string checkExistenceQuery = std::string()
		+ "SELECT id, awaits_creation FROM entities "
		+ "WHERE name=" + work.quote (name) + ";";
	auto result = work.exec (checkExistenceQuery);
	
	int id;

	if (result.size() > 1) {
		throw std::logic_error (
			std::string ("CreatePageResource::processRequest::checkExistenceQuery returned too many rows for name='") + name + "'"
		);
	} else if (result.size() == 1) {
		id = result[0][0].as <int> ();
		bool awaitsCreation = result[0][1].as <bool> ();

		if (!awaitsCreation) return -1;
		std::string updateEntityQuery =
			"UPDATE entities "s
			+ "SET type=" + work.quote (type) + ","
			+ "name=" + work.quote (name) + ","
			+ "description=" + work.quote (desc) + ","
			+ "awaits_creation=FALSE,"
			+ "created_by=" + std::to_string (uid) + ","
			+ "created_on=CURRENT_DATE,"
			+ "start_date=" + work.quote (startDate) + ","
			+ "end_date=" + ((endDate == "") ? "NULL" : work.quote(endDate)) 
			+ " WHERE id=" + std::to_string (id) + ";";
		work.exec (updateEntityQuery);

	} else /* result.size() == 0 */ {
		std::string insertEntityQuery = 
			"INSERT INTO entities"s
			"(type, name, description, awaits_creation, created_by, created_on, start_date, end_date)"
			"VALUES ("
			+ /* type */ 		work.quote (type) + ","
			+ /* name */		work.quote (name) + ","
			+ /* desc */		work.quote (desc) + ","
			+ /* awaits_creat*/ "FALSE,"
			+ /* created_by */ 	std::to_string (uid) + ","
			+ /* created_on */  "CURRENT_DATE,"
			+ /* start_date */	work.quote (startDate) + ","
			+ /* end_date */ ((endDate == "") ? "NULL" : work.quote(endDate))
			+ ") RETURNING id;";
		auto insertion = work.exec (insertEntityQuery);
		id = insertion[0][0].as <int> ();
	}

	return id;
}

int CreatePageResource::createEmptyEntityWithWork (OwnedConnection &work, const std::string &name) {
	std::string insertEmptyEntityQuery = "INSERT INTO entities (name) VALUES (" + work.quote (name) + ") RETURNING id;";
	auto res = work.exec1 (insertEmptyEntityQuery);
	return res[0].as <int>();
}

int CreatePageResource::createTypedEntity (OwnedConnection &work, int entityId, std::string type) {
	const std::map <std::string /* type */, std::string /* table */> typeToTableName = {
		{"person", "personalities"},
		{"band", "bands"},
		{"album", "albums"}
	};
	std::string tableName = typeToTableName.at (type);
	
	std::string createRowQuery = std::string()
		+ "INSERT INTO " + tableName /* no escape - safe table name */ + " (entity_id)"
		+ "VALUES (" + std::to_string (entityId) + ") RETURNING id;";
	pqxx::result result = work.exec (createRowQuery);
	return result[0][0].as <int>();
}

std::string CreatePageResource::pageUrlForTypedEntity (std::string type, int id) {
	return "/e?id=" + std::to_string(id);
}

/* static */ std::string CreatePageResource::urlForId (int id) {
	return "/e?id=" + std::to_string(id);
}

std::unique_ptr<ApiResponse> CreatePageResource::processRequest (RequestData &rd, nlohmann::json body) {
	assertMethod (rd, "POST");

	auto &userManager = UserManager::get();

	auto user = userManager.getUserDataBySessionId (rd.getCookie(SESSION_ID));
	if (!user.valid()) return makeApiResponse (nlohmann::json{}, 401);

	std::string type, name, description, startDate, endDate;

	type = getParameter <std::string> ("type", body);
	name = getParameter <std::string> ("name", body);
	description = getParameter <std::string> ("description", body);
	startDate = getParameter <std::string> ("start_date", body);
	if (body.contains ("end_date")) {
		endDate = getParameter <std::string> ("end_date", body);
	}
	

	const std::set <std::string> allowedTypes = { "person", "band" };
	if (!allowedTypes.contains (type)) return makeApiResponse (nlohmann::json{}, 400);
	
	auto conn = database.connect ();

	// Check if already mentioned
	int entityId = this->createEntity (conn, type, name, description, startDate, endDate, user.id());
	if (entityId < 0) {
		return makeApiResponse (nlohmann::json{{"status", "already_exists"}}, 200);
	}
	this->createTypedEntity (conn, entityId, type);

	std::string url = CreatePageResource::pageUrlForTypedEntity (type, entityId);

	auto &resourceManager = UploadedResourcesManager::get();
	auto resourceData = resourceManager.createUploadableResource ();

	std::string setPicResourceQuery = 
		"UPDATE entities SET picture="s + std::to_string(resourceData.resourceId) + " WHERE id=" + std::to_string (entityId) + ";";
	conn.exec (setPicResourceQuery);

	conn.commit ();

	int searchIndex = updateEntityIndex (entityId);
	auto &searchMgr = SearchManager::get();
	searchMgr.setPictureForResource (searchIndex, resourceData.resourceId);

	logger << "Создана страница '" << name << "'" << std::endl;
	auto response = makeApiResponse (
		nlohmann::json {
			{"status", "success"},
			{"entity", entityId},
			{"url", url},
			{"upload_url",  resourceManager.getUploadUrl(resourceData.uploadId)}
		},
		201
	);
	return response;
}



RequestPictureChangeResource::RequestPictureChangeResource (mg_context *ctx, std::string uri, std::string uploadUrl):
ApiResource (ctx, uri), _uploadUrl (uploadUrl) {

}

ApiResponsePtr RequestPictureChangeResource::processRequest (RequestData &rd, nlohmann::json body) {
	if (rd.method != "POST") return makeApiResponse (405);

	auto &userManager = UserManager::get();
	auto user = userManager.getUserDataBySessionId (rd.getCookie(SESSION_ID));
	if (!user.valid()) return makeApiResponse (nlohmann::json{}, 401);

	int entityId = getParameter <int> ("entity_id", body);
	// Check that entity exists
	if (!EntityDataResource::entityCreated (entityId))
		return makeApiResponse (nlohmann::json{{"status", "entity_not_found"}}, 404);
	
	std::string getResourceId = "SELECT picture FROM entities WHERE id="s + std::to_string (entityId) + ";";
	auto conn = database.connect();
	auto row = conn.exec1 (getResourceId);
	int resourceId = row.at(0).as <int>();

	auto &resourceManager = UploadedResourcesManager::get();
	std::string uploadId = resourceManager.generateUploadIdForResource (resourceId);
	
	conn.commit();
	return makeApiResponse (nlohmann::json {{"url", resourceManager.getUploadUrl(uploadId)}});
}




EntityDataResource::EntityDataResource (mg_context *ctx, std::string uri, std::string picsUri):
ApiResource (ctx, uri), _pics (picsUri) {

}

bool EntityDataResource::entityCreated (int id) {
	std::string query = "SELECT 1 FROM entities WHERE awaits_creation=FALSE AND id=" + std::to_string (id) + ";";
	auto conn = database.connect();
	auto res = conn.exec (query);
	return res.size() > 0;
}

int EntityDataResource::getEntityByNameWithWork (OwnedConnection &work, const std::string &name) {
	std::string checkEntityExistenceQuery = "SELECT id FROM entities WHERE name="s + work.quote (name) + ";";
	auto checkRes = work.exec (checkEntityExistenceQuery);
	if (checkRes.size() == 0) return CreatePageResource::createEmptyEntityWithWork (work, name);
	return checkRes[0][0].as <int> ();
}

int EntityDataResource::getEntityByName (const std::string &name) {
	auto conn = database.connect();
	auto id = EntityDataResource::getEntityByNameWithWork (conn, name);
	conn.commit();
	return id;
}


EntityData EntityDataResource::getEntityDataByIdWithWork (OwnedConnection &work, int id) {
	EntityData ed;
	std::string getEntityDataByIdQuery = 
		"SELECT name, description, type, start_date, end_date, path, awaits_creation, created_by, created_on "
		"FROM entities LEFT OUTER JOIN uploaded_resources ON entities.picture=uploaded_resources.id "
		"WHERE entities.id="s + std::to_string (id) + ";";
	auto result = work.exec (getEntityDataByIdQuery);
	if (result.size() == 0) {
		ed.valid = false;
		return ed;
	} else {
		auto row = result[0];
		ed.valid = true;

		ed.id = id;
		ed.name = row ["name"].as <std::string>();
		ed.description = row ["description"].as <std::string>();
		ed.type = row ["type"].as <std::string>();

		ed.startDate = row ["start_date"].as <std::string>();
		if (!row["end_date"].is_null()) ed.endDate = row["end_date"].as <std::string>();
		
		ed.created = !(row["awaits_creation"].as <bool>());

		if (!row.at("path").is_null()) {
			UploadedResourcesManager &mgr = UploadedResourcesManager::get();
			ed.picturePath = mgr.getDownloadUrl(row.at("path").as <std::string>());
		}
		if (!row["created_by"].is_null()) ed.createdBy = row["created_by"].as <int>();
		if (!row["created_on"].is_null()) ed.createdOn = row["created_on"].as <std::string>();

		return ed;
	}
}

EntityData EntityDataResource::getEntityDataById (int id) {
	auto conn = database.connect();
	return EntityDataResource::getEntityDataByIdWithWork (conn, id);
}

ApiResponsePtr EntityDataResource::processRequest (RequestData &rd, nlohmann::json body) {
	if (rd.method != "POST") return makeApiResponse (nlohmann::json{}, 405);
	if (!body.contains ("id")) return makeApiResponse (nlohmann::json{}, 400);

	auto &userManager = UserManager::get();
	auto user = userManager.getUserDataBySessionId (rd.getCookie(SESSION_ID));


	int id = getParameter <int> ("id", body);

	// std::string getEntityDataQuery = std::string()
	// 	+ "SELECT type, name, description, start_date, end_date, picture_path, awaits_creation, created_by "
	// 	+ "FROM entities "
	// 	+ "WHERE id=" + std::to_string (id) + ";";

	// auto conn = database.connect();

	// pqxx::result result = conn.exec (getEntityDataQuery);
	auto response = makeApiResponse ();

	auto data = this->getEntityDataById (id);

	// if (result.size() == 0) {
	if (data.valid == false) {
		response->body ["redirect"] = std::string("/");
		return response;
	}
	// if (result.size() > 1) throw std::logic_error ("EntityDataResource::processRequest found duplicate ids");
	
	// pqxx::row row = result[0];

	// if (row["awaits_creation"].as <bool>()) {
	if (!data.created) {
		if (user.valid()) {
			response->body ["redirect"] = std::string("/create?q=") + queryString (data.name);
		} else {
			response->body ["redirect"] = std::string("/");
		}
		return response;
	}


	response->body ["entity_id"] = id;

	// response->body ["name"] = row ["name"].as <std::string>();


	// response->body ["description"] = row ["description"].as <std::string>();
	// response->body ["start_date"] = row ["start_date"].as <std::string>();
	// response->body ["type"] = row ["type"].as <std::string>();
	// if (!row ["end_date"].is_null()) response->body ["end_date"] = row ["end_date"].as <std::string>();
	// if (!row ["picture_path"].is_null()) response->body ["picture_path"] = this->_pics + "/" + row ["picture_path"].as <std::string>();
	// response->body ["created_by"] = row ["created_by"].as <int>();


	response->body ["name"] = data.name;


	response->body ["description"] = data.description;
	response->body ["start_date"] = data.startDate;
	response->body ["type"] = data.type;
	if (data.endDate.has_value()) response->body ["end_date"] = data.endDate.value();
	if (data.picturePath.has_value()) response->body ["picture_path"] = data.picturePath.value();
	if (data.createdBy.has_value()) response->body ["created_by"] = data.createdBy.value();
	return response;
}
