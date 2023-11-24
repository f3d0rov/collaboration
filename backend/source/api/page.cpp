
#include "page.hpp"

CreatePageResource::CreatePageResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

int CreatePageResource::createEntity (OwnedConnection &work, std::string type, std::string name, std::string desc, std::string startDate, std::string endDate, int uid) {
	std::string checkExistenceQuery = std::string()
		+ "SELECT id, awaits_creation FROM entities "
		+ "WHERE name=" + work.quote (name) + ";";
	auto result = work.exec (checkExistenceQuery);
	
	if (result.size() > 1) {
		throw std::logic_error (
			std::string ("CreatePageResource::processRequest::checkExistenceQuery returned too many rows for name='") + name + "'"
		);
	} else if (result.size() == 1) {
		int id = result[0][0].as <int> ();
		bool awaitsCreation = result[0][1].as <bool> ();

		if (!awaitsCreation) return -1;
		std::string updateEntityQuery = std::string ("UPDATE entities ")
			+ "SET type=" + work.quote (type) + ","
			+ "name=" + work.quote (escapeHTML(name)) + ","
			+ "description=" + work.quote (escapeHTML(desc)) + ","
			+ "awaits_creation=FALSE,"
			+ "created_by=" + std::to_string (uid) + ","
			+ "created_on=CURRENT_DATE,"
			+ "start_date=" + work.quote (startDate) + ","
			+ "end_date=" + ((endDate == "") ? "NULL" : work.quote(endDate)) 
			+ " WHERE id=" + std::to_string (id) + ";";
		work.exec (updateEntityQuery);
		return id;

	} else /* result.size() == 0 */ {
		std::string insertEntityQuery = 
			std::string("INSERT INTO entities (")
			+ "type, name, description, awaits_creation, created_by, created_on, start_date, end_date"
			+ ") VALUES ("
			+ /* type */ 		work.quote (type) + ","
			+ /* name */		work.quote (escapeHTML(name)) + ","
			+ /* desc */		work.quote (escapeHTML(desc)) + ","
			+ /* awaits_creat*/ "FALSE,"
			+ /* created_by */ 	std::to_string (uid) + ","
			+ /* created_on */  "CURRENT_DATE,"
			+ /* start_date */	work.quote (startDate) + ","
			+ /* end_date */ ((endDate == "") ? "NULL" : work.quote(endDate))
			+ ") RETURNING id;";
		auto insertion = work.exec (insertEntityQuery);
		return insertion[0][0].as <int> ();
	}
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

std::unique_ptr<ApiResponse> CreatePageResource::processRequest (RequestData &rd, nlohmann::json body) {
	logger << "Создаем страницу" << std::endl;
	if (rd.method != "POST") return makeApiResponse (nlohmann::json{}, 405);

	auto &userManager = UserManager::get();

	auto user = userManager.getUserDataBySessionId (rd.getCookie(SESSION_ID));
	if (!user.valid()) return makeApiResponse (nlohmann::json{}, 401);

	if (!(
		   body.contains ("type")
		&& body.contains ("name")
		&& body.contains ("description")
		&& body.contains ("start_date")
	)) return makeApiResponse (nlohmann::json{}, 400);

	std::string type, name, description, startDate, endDate;
	bool hasEndDate = false;

	type = getParameter <std::string> ("type", body);
	name = getParameter <std::string> ("name", body);
	description = getParameter <std::string> ("description", body);
	startDate = getParameter <std::string> ("start_date", body);
	if (body.contains ("end_date")) {
		endDate = getParameter <std::string> ("end_date", body);
		hasEndDate = true;
	}
	

	const std::set <std::string> allowedTypes = { "person", "band" };
	if (!allowedTypes.contains (type)) return makeApiResponse (nlohmann::json{}, 400);
	
	auto conn = database.connect ();

	// Check if already mentioned
	int entityId = this->createEntity (conn, type, name, description, startDate, endDate, user.id());
	if (entityId < 0) {
		return makeApiResponse (nlohmann::json{{"status", "already_exists"}}, 200);
	}
	int pageId = this->createTypedEntity (conn, entityId, type);

	std::string url = CreatePageResource::pageUrlForTypedEntity (type, entityId);
	// Index created page
	Searcher::indexWithWork (conn, type, url, name + " " + description, name, description, "");

	conn.commit ();
	logger << "Создана страница '" << name << "'" << std::endl;
	auto response = makeApiResponse (
		nlohmann::json {
			{"status", "success"},
			{"entity", entityId},
			{"url", url}
		},
		200
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

	std::string id = randomizer.hex (128);
	
	auto conn = database.connect();

	std::string createRequest = std::string("INSERT INTO entity_photo_upload_links (id, entity_id, valid_until) ")
		+ "VALUES ("
			/* id */ 			+ conn.quote (id) + ","
			/* entity_id */		+ std::to_string (entityId) + ","
			/* valid_until */	+ "CURRENT_TIMESTAMP + INTERVAL '5 minutes');";
	auto result = conn.exec (createRequest);
	conn.commit();
	return makeApiResponse (nlohmann::json {{"url", "/" + this->_uploadUrl + "?id=" + id}});
}



UploadPictureResource::UploadPictureResource (mg_context *ctx, std::string uri, std::filesystem::path savePath):
Resource (ctx, uri), _savePath (savePath) {

}

void UploadPictureResource::setEntityPicture (OwnedConnection &work, int entityId, std::string path) {
	std::string checkForPic = std::string ()
		+ "SELECT picture_path FROM entities WHERE id=" + std::to_string (entityId) + ";";
	auto res = work.exec1 (checkForPic);
	if (!res[0].is_null()) {
		std::filesystem::path oldPath = this->_savePath / std::filesystem::path(res[0].as <std::string>());
		std::error_code ec;
		std::filesystem::remove (oldPath, ec);
		if (ec) {
			logger << "Ошибка при попытке удалить файл: '" << oldPath << "' #" << ec << std::endl;
		}
	}
	std::string setPicPath = std::string()
		+ "UPDATE entities SET picture_path=" + work.quote (path) + " WHERE id=" + std::to_string (entityId) + ";";
	auto updateRes = work.exec (setPicPath);
	if (updateRes.affected_rows() != 1) throw std::runtime_error ("UploadPictureResource::setEntityPicture::updateRes.affected_rows() != 1");
}

std::unique_ptr <_Response> UploadPictureResource::processRequest (RequestData &rd) {
	if (rd.method != "PUT") return std::make_unique <Response> (405);

	auto &userManager = UserManager::get();
	auto user = userManager.getUserDataBySessionId (rd.getCookie(SESSION_ID));
	if (!user.valid()) return makeApiResponse (nlohmann::json{}, 401);

	auto conn = database.connect();

	std::string id = rd.query ["id"];
	std::string checkRequest = std::string ()
		+ "SELECT entity_id FROM entity_photo_upload_links "
			"WHERE id=" + conn.quote (id) + " AND valid_until > CURRENT_TIMESTAMP;";
	auto result = conn.exec (checkRequest);

	if (result.size() == 0) return makeApiResponse (404);
	int entityId = result[0][0].as <int>();


	std::string headerEnd = ";base64,";
	int origin = rd.body.find (headerEnd) + headerEnd.length();
	logger << "pos: " << origin << std::endl;

	int mimeStart = rd.body.find ("data:");
	std::string mime = rd.body.substr (mimeStart + 5, rd.body.find (";", mimeStart + 5) - mimeStart - 5);
	logger << mime << std::endl;
	
	size_t decodedLen = rd.body.length() * 3 / 4;
	unsigned char *buffer = new unsigned char[decodedLen];
	if (-1 != mg_base64_decode (rd.body.c_str() + origin, rd.body.length() - origin, buffer, &decodedLen)) {
		logger << "UploadPictureResource::processRequest не смог декодировать данные в формате Base64" << std::endl;
		return std::make_unique <Response> (500);
	}

	std::filesystem::path filename = std::filesystem::path (randomizer.hex(64) + ".png");
	std::ofstream of (this->_savePath / filename, std::ios::binary | std::ios::out);
	if (!of.is_open()) {
		logger << "Не смог открыть файл для записи: " << filename << std::endl;
		return std::make_unique <Response> (500);
	}

	of.write ((const char *) buffer, decodedLen);
	of.close();
	delete[] buffer;

	logger << "Фото сущности сохранено в: " << filename << std::endl;
	this->setEntityPicture (conn, entityId, filename);
	conn.commit();

	return std::make_unique <Response> (200);
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
		"SELECT name, description, type, start_date, end_date, picture_path, awaits_creation, created_by, created_on FROM entities WHERE id="s + std::to_string (id) + ";";
	auto result = work.exec (getEntityDataByIdQuery);
	if (result.size() == 0) {
		ed.valid = false;
		return ed;
	} else {
		auto row = result[0];

		ed.id = id;
		ed.name = row ["name"].as <std::string>();
		ed.description = row ["description"].as <std::string>();
		ed.type = row ["type"].as <std::string>();

		ed.startDate = row ["start_date"].as <std::string>();
		if (!row["end_date"].is_null()) ed.endDate = row["end_date"].as <std::string>();
		
		ed.created = !(row["awaits_creation"].as <bool>());

		if (!row["picture_path"].is_null()) ed.picturePath = row["picture_path"].as <std::string>();
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


	int id;
	try {
		id = body["id"].get <int>();
	} catch (nlohmann::json::exception &e) {
		return makeApiResponse (nlohmann::json{}, 400);
	}

	std::string getEntityDataQuery = std::string()
		+ "SELECT type, name, description, start_date, end_date, picture_path, awaits_creation, created_by "
		+ "FROM entities "
		+ "WHERE id=" + std::to_string (id) + ";";

	auto conn = database.connect();

	pqxx::result result = conn.exec (getEntityDataQuery);
	auto response = makeApiResponse ();

	if (result.size() == 0) {
		response->body ["redirect"] = std::string("/");
		return response;
	}
	if (result.size() > 1) throw std::logic_error ("EntityDataResource::processRequest found duplicate ids");
	
	pqxx::row row = result[0];

	if (row["awaits_creation"].as <bool>()) {
		if (user.valid()) {
			response->body ["redirect"] = std::string("/create?q=") + queryString (row["name"].as <std::string>());
		} else {
			response->body ["redirect"] = std::string("/");
		}
		return response;
	}


	response->body ["entity_id"] = id;
	response->body ["name"] = row ["name"].as <std::string>();


	response->body ["description"] = row ["description"].as <std::string>();
	response->body ["start_date"] = row ["start_date"].as <std::string>();
	response->body ["type"] = row ["type"].as <std::string>();
	if (!row ["end_date"].is_null()) response->body ["end_date"] = row ["end_date"].as <std::string>();
	if (!row ["picture_path"].is_null()) response->body ["picture_path"] = this->_pics + "/" + row ["picture_path"].as <std::string>();
	response->body ["created_by"] = row ["created_by"].as <int>();

	return response;
}
