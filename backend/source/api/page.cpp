
#include "page.hpp"

CreatePageResource::CreatePageResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

int CreatePageResource::createEntity (pqxx::work &work, std::string type, std::string name, std::string desc, std::string startDate, std::string endDate, int uid) {
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
			+ "name=" + work.quote (name) + ","
			+ "description=" + work.quote (desc) + ","
			+ "awaits_creation=FALSE,"
			+ "created_by=" + std::to_string (uid) + ","
			+ "created_on=CURRENT_DATE,"
			+ "start_date=" + work.quote (startDate) + ","
			+ "endDate=" + ((endDate == "") ? "NULL" : work.quote(endDate)) 
			+ "WHERE id=" + std::to_string (id) + ";";
		work.exec (updateEntityQuery);
		return id;

	} else /* result.size() == 0 */ {
		logger << "NEW" << std::endl;
		std::string insertEntityQuery = 
			std::string("INSERT INTO entities (")
			+ "type, name, description, awaits_creation, created_by, created_on, start_date, end_date"
			+ ") VALUES ("
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
		return insertion[0][0].as <int> ();
	}
}

int CreatePageResource::createTypedEntity (pqxx::work &work, int entityId, std::string type) {
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
	const std::map <std::string /* type */, std::string /* url */> urls = {
		{"person", "p"},
		{"band", "b"},
		{"album", "a"}
	};
	return "/" + urls.at (type) + "?id=" + std::to_string(id);
}

std::unique_ptr<ApiResponse> CreatePageResource::processRequest (RequestData &rd, nlohmann::json body) {
	logger << "Создаем страницу" << std::endl;
	if (rd.method != "POST") return std::make_unique <ApiResponse> (nlohmann::json{}, 405);

	if (!rd.setCookies.contains ("session_id")) return std::make_unique <ApiResponse> (nlohmann::json{}, 401);
	auto user = CheckSessionResource::checkSessionId (rd.setCookies["session_id"]);
	if (!user.valid) return std::make_unique <ApiResponse> (nlohmann::json{}, 401);

	if (!(
		   body.contains ("type")
		&& body.contains ("name")
		&& body.contains ("description")
		&& body.contains ("start_date")
	)) return std::make_unique <ApiResponse> (nlohmann::json{}, 400);

	std::string type, name, description, startDate, endDate;
	bool hasEndDate = false;

	try {
		type = body ["type"].get <std::string>();
		name = body ["name"].get <std::string>();
		description = body ["description"].get <std::string>();
		startDate = body ["start_date"].get <std::string>();
		if (body.contains ("end_date")) {
			hasEndDate = true;
			endDate = body ["end_date"].get <std::string>();
		}
	} catch (nlohmann::json::exception &e) {
		return std::make_unique <ApiResponse> (nlohmann::json{}, 400);
	}

	const std::set <std::string> allowedTypes = { "person", "band", "album" };
	if (!allowedTypes.contains (type)) return std::make_unique <ApiResponse> (nlohmann::json{}, 400);
	
	auto conn = database.connect ();
	pqxx::work work (*conn.conn);

	// Check if already mentioned
	int entityId = this->createEntity (work, type, name, description, startDate, endDate, user.uid);
	if (entityId < 0) {
		return std::make_unique <ApiResponse> (nlohmann::json{{"status", "already_exists"}}, 200);
	}
	int pageId = this->createTypedEntity (work, entityId, type);

	std::string url = CreatePageResource::pageUrlForTypedEntity (type, pageId);
	// Index created page
	SearchResource::indexWithWork (work, type, url, name + " " + description, name, description, "");

	work.commit ();
	logger << "Создана страница '" << name << "'" << std::endl;
	auto response = std::make_unique <ApiResponse> (
		nlohmann::json {
			{"status", "success"},
			{"url", url}
		},
		200
	);
	return response;
}

EntityDataResource::EntityDataResource (mg_context *ctx, std::string uri, std::string entityTable):
ApiResource (ctx, uri), _table (entityTable) {

}

std::unique_ptr <ApiResponse> EntityDataResource::processRequest (RequestData &rd, nlohmann::json body) {
	if (rd.method != "POST") return std::make_unique <ApiResponse> (nlohmann::json{}, 405);
	if (!body.contains ("id")) return std::make_unique <ApiResponse> (nlohmann::json{}, 400);

	int id;
	try {
		id = body["id"].get <int>();
	} catch (nlohmann::json::exception &e) {
		return std::make_unique <ApiResponse> (nlohmann::json{}, 400);
	}

	std::string getEntityDataQuery = std::string()
		+ "SELECT entities.id, name, description, start_date, end_date, picture_path, awaits_creation, created_by "
		+ "FROM entities INNER JOIN " + this->_table + " on entities.id = " + this->_table + ".entity_id "
		+ "WHERE " + this->_table + ".id=" + std::to_string (id) + ";";

	auto conn = database.connect();
	pqxx::work work (*conn.conn);

	pqxx::result result = work.exec (getEntityDataQuery);
	if (result.size() == 0) return std::make_unique <ApiResponse> (nlohmann::json {{"status", "not_found"}}, 404);
	if (result.size() > 1) throw std::logic_error ("EntityDataResource::processRequest found duplicate ids");
	
	pqxx::row row = result[0];
	auto response = std::make_unique <ApiResponse> ();
	// bool awaitsCreation = row ["awaits_creation"].as <bool>();
	
	// for (int i = 0; i < row.size(); i++) {
	// 	logger << "'" << row[i].name() << "': " << row[i].c_str() << std::endl;
	// }

	response->body ["entity_id"] = row ["id"].as <std::string>();
	// response->body ["awaits_creation"] = awaitsCreation;
	response->body ["name"] = row ["name"].as <std::string>();

	// if (awaitsCreation) return response;

	response->body ["description"] = row ["description"].as <std::string>();
	response->body ["start_date"] = row ["start_date"].as <std::string>();
	if (!row ["end_date"].is_null()) response->body ["end_date"] = row ["end_date"].as <std::string>();
	if (!row ["picture_path"].is_null()) response->body ["picture_path"] = row ["picture_path"].as <std::string>();
	response->body ["created_by"] = row ["created_by"].as <int>();

	return response;
}
