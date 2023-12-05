
#include "entity_manager.hpp"


EntityTypeInterface::~EntityTypeInterface () = default;



// All functions do nothing by default
int EntityTypeInterface::createEntity (OwnedConnection &conn, int entityId, const nlohmann::json &data) { return entityId; }
nlohmann::json EntityTypeInterface::getEntity (OwnedConnection &conn, int entityId) { return nlohmann::json{}; }
int EntityTypeInterface::updateEntity (OwnedConnection &conn, int entityId, const nlohmann::json &data) { return entityId; }
void EntityTypeInterface::deleteEntity (OwnedConnection &conn, int entityId) {}

void EntityTypeInterface::indexEntity (OwnedConnection &conn, int entityId) {}


std::string EntityTypeInterface::getTitleString () const {
	return "Имя";
}


std::string EntityTypeInterface::getEndDateString () const {
	return "";
}


bool EntityTypeInterface::hasEndDate () const {
	return false;
}

bool EntityTypeInterface::squareImage () const {
	return true;
}




std::unique_ptr <EntityManager> EntityManager::_manager;

EntityManager::EntityManager () {

}

/* static */ EntityManager &EntityManager::get () {
	if (EntityManager::_manager.get() == nullptr)
		EntityManager::_manager = std::unique_ptr <EntityManager> (new EntityManager{}); //? unsafe?
	return *EntityManager::_manager; 
}

bool EntityManager::entityWithNameExists (OwnedConnection &conn, std::string name) {
	std::string query = "SELECT 1 FROM entities WHERE name = " + conn.quote (name) + ";";
	return conn.exec (query).size() != 0;
}

bool EntityManager::entityCreated (OwnedConnection &conn, std::string name) {
	std::string query = "SELECT awaits_creation FROM entities WHERE name = " + conn.quote (name) + ";";
	return conn.exec1 (query).at (0).as <bool>() != true;
}

bool EntityManager::entityCreated (OwnedConnection &conn, int entityId) {
	std::string query = "SELECT awaits_creation FROM entities WHERE id = " + std::to_string (entityId) + ";";
	return conn.exec1 (query).at (0).as <bool>() != true;
}

int EntityManager::getEntityIdByName (OwnedConnection &conn, std::string name) {
	std::string query = "SELECT id FROM entities WHERE name = " + conn.quote (name) + ";";
	return conn.exec1 (query).at (0).as <int>();
}

std::optional <int> EntityManager::maybeGetEntityIdByName (OwnedConnection &conn, std::string name) {
	std::string query = "SELECT id FROM entities WHERE name = " + conn.quote (name) + ";";
	auto result = conn.exec (query);
	if (result.size() == 0) return {};
	return result.at (0).at (0).as <int>();
}

/* static */ std::string EntityManager::urlForEntity (int entityId) {
	return "/e?id=" + std::to_string (entityId);
}



bool EntityManager::entityWasIndexed (OwnedConnection &conn, int entityId) {
	std::string getIndexQuery =
		"SELECT search_resource "
		"FROM entities "
		"WHERE id="s + std::to_string (entityId) + ";";

	auto row = conn.exec1 (getIndexQuery);
	return row.at (0).is_null() == false;
}

int EntityManager::getEntityIndexResId (OwnedConnection &conn, int entityId) {
	std::string getIndexQuery =
		"SELECT search_resource "
		"FROM entities "
		"WHERE id="s + std::to_string (entityId) + ";";

	auto row = conn.exec1 (getIndexQuery);
	return row.at (0).as <int> ();
}

int EntityManager::createEntityIndex (OwnedConnection &conn, int entityId) {
	// auto &mgr = SearchManager::get();
	// int index = mgr.indexNewResource (
	// 	entityId,
	// 	EntityManager::urlForEntity (entityId),
	// 	data.name,
	// 	data.description,
	// 	data.type
	// );
	// return index;
	return -1;
}

int EntityManager::clearEntityIndex (OwnedConnection &conn, int entityId) {
	auto &mgr = SearchManager::get();
	int index = this->getEntityIndexResId (conn, entityId);
	// !!! VERY IMPORTANT !!!
	// TODO: DONT FORGET THIS
	// !!! VERY IMPORTANT !!!
	// mgr.updateResourceData (index, data.name, data.description);
	mgr.clearIndexForResource (index);
	return index;
}

void EntityManager::indexStringForEntity (OwnedConnection &conn, int indexId, std::string str, int value) {
	auto &mgr = SearchManager::get();
	mgr.indexStringForResource (indexId, str, value);
}


void EntityManager::registerType (std::shared_ptr <EntityTypeInterface> entityType) {
	std::unique_lock lockForTypesUpdate(this->_typesModificationMutex); // Using `unique_lock` for unique access.
	// Threads that access `_types` but don't modify it must use shared_lock to allow simultaneous use and
	// protection agains race conditions (even though `_types` are not expected to change once the program is initialized).
	// In case a concrete type implementation requires that only one thread can access it or its parts at once
	// it will have to provide a separate mutex.

	if (this->_types.contains (entityType->getTypeId()))
		throw std::logic_error ("EntityManager: регистрация типа с уже зарегистрированным id = '"s + entityType->getTypeId() + "'");

	this->_types.insert (std::make_pair (entityType->getTypeId(), entityType));
}

int EntityManager::completeEntity (OwnedConnection &conn, int entityId, EntityManager::BasicEntityData entityData, int byUser) {
	std::string updateEntityQuery =
		"UPDATE entities "s
		+ "SET type=" + conn.quote (entityData.type) + ","
		+ "name=" + conn.quote (entityData.name) + ","
		+ "description=" + conn.quote (entityData.description) + ","
		+ "awaits_creation=FALSE,"
		+ "created_by=" + std::to_string (byUser) + ","
		+ "created_on=CURRENT_DATE,"
		+ "start_date=" + conn.quote (entityData.startDate) + ","
		+ "end_date=" + (entityData.endDate.has_value() ? "NULL" : conn.quote(entityData.endDate.value())) 
		+ " WHERE id=" + std::to_string (entityId) + ";";
	conn.exec (updateEntityQuery);
	return entityId;
}

int EntityManager::createNewEntity (OwnedConnection &conn, EntityManager::BasicEntityData entityData, int byUser) {
	std::string insertEntityQuery = 
		"INSERT INTO entities"s
		"(type, name, description, awaits_creation, created_by, created_on, start_date, end_date)"
		"VALUES ("
		+ /* type */ 		conn.quote (entityData.type) + ","
		+ /* name */		conn.quote (entityData.name) + ","
		+ /* desc */		conn.quote (entityData.description) + ","
		+ /* awaits_creat*/ "FALSE,"
		+ /* created_by */ 	std::to_string (byUser) + ","
		+ /* created_on */  "CURRENT_DATE,"
		+ /* start_date */	conn.quote (entityData.startDate) + ","
		+ /* end_date */ (entityData.endDate.has_value() ? "NULL" : conn.quote(entityData.endDate.value())) 
		+ ") RETURNING id;";
	auto insertion = conn.exec1 (insertEntityQuery);
	return insertion.at(0).as <int> ();
}


EntityManager::ExtendedEntityData EntityManager::getEntityData (OwnedConnection &conn, int entityId) {
	EntityManager::ExtendedEntityData ed;
	std::string getEntityDataByIdQuery = 
		"SELECT name, description, type, start_date, end_date, path, awaits_creation, created_by, created_on "
		"FROM entities LEFT OUTER JOIN uploaded_resources ON entities.picture=uploaded_resources.id "
		"WHERE entities.id="s + std::to_string (entityId) + ";";
	auto result = conn.exec (getEntityDataByIdQuery);

	if (result.size() == 0) {
		throw UserMistakeException ("Сущность не существует", 404, "not_found");
	} else {
		auto row = result[0];
		ed.id = entityId;
		ed.created = !(row.at ("awaits_creation").as <bool>());
		ed.name = row ["name"].as <std::string>();

		if (ed.created) {
			ed.type = row ["type"].as <std::string>();
			ed.description = row ["description"].as <std::string>();

			ed.startDate = row ["start_date"].as <std::string>();
			if (!row["end_date"].is_null()) ed.endDate = row["end_date"].as <std::string>();
			
			if (!row.at("path").is_null()) {
				UploadedResourcesManager &mgr = UploadedResourcesManager::get();
				ed.pictureUrl = mgr.getDownloadUrl(row.at("path").as <std::string>());
			}

			if (!row["created_by"].is_null()) ed.createdBy = row["created_by"].as <int>();
			if (!row["created_on"].is_null()) ed.createdOn = row["created_on"].as <std::string>();

			std::shared_lock dontModifyTypesPlease (this->_typesModificationMutex);
			ed.typeData = this->_types.at (ed.type)->getEntity (conn, entityId);
		}

		return ed;
	}
}


int EntityManager::createEntity (EntityManager::BasicEntityData entityData, int byUser) {
	std::shared_lock dontModifyTypes (this->_typesModificationMutex);
	auto conn = database.connect();

	if (this->_types.contains (entityData.type) == false)
		throw UserMistakeException ("Тип сущности '"s + entityData.type + "' не зарегистрирован", 400, "bad_type");
	
	auto type = this->_types.at (entityData.type);
	auto id = this->maybeGetEntityIdByName (conn, entityData.name);
	int finalId = 0;
	if (id.has_value()) {
		if (this->entityCreated (conn, id.value())) throw UserMistakeException ("Сущность уже создана!", 409, "already_created");
		finalId = this->completeEntity (conn, id.value(), entityData, byUser);
	} else {
		finalId = this->createNewEntity (conn, entityData, byUser);
	}

	nlohmann::json nothing{};
	type->createEntity (conn, finalId, entityData.typeData.has_value() ? entityData.typeData.value() : nothing); // yeah.

	int index = this->createEntityIndex (conn, finalId);
	this->indexStringForEntity (conn, index, entityData.name, SEARCH_VALUE_TITLE);
	this->indexStringForEntity (conn, index, entityData.description, SEARCH_VALUE_DESCRIPTION);
	type->indexEntity (conn, finalId);

	conn.commit();
	return finalId;
}

EntityManager::ExtendedEntityData EntityManager::getEntity (int entityId) {
	std::shared_lock dontModifyTypes (this->_typesModificationMutex);
	auto conn = database.connect();
	return this->getEntityData (conn, entityId);
}

int EntityManager::updateEntity (nlohmann::json &json, int byUser) {
	std::shared_lock dontModifyTypes (this->_typesModificationMutex);
	return -1;
}

void EntityManager::deleteEntity (int entityId, int byUser) {
	std::shared_lock dontModifyTypes (this->_typesModificationMutex);

}


void to_json (nlohmann::json &j, const EntityManager::BasicEntityData &d) {
	j ["type"] = d.type;
	j ["name"] = d.name;
	j ["description"] = d.description;
	j ["start_date"] = d.startDate;
	if (d.endDate.has_value()) j ["end_date"] = d.endDate.value();
}

void from_json (const nlohmann::json &j, EntityManager::BasicEntityData &d) {
	d.type = getParameter <std::string> ("type", j);
	d.name = getParameter <std::string> ("name", j);
	d.description = getParameter <std::string> ("description", j);
	d.startDate = getParameter <std::string> ("start_date", j);
	if (j.contains ("end_date")) d.endDate = getParameter <std::string> ("end_date", j);
}

void to_json (nlohmann::json &j, const EntityManager::ExtendedEntityData &d) {
	to_json (j, static_cast <EntityManager::BasicEntityData> (d));
	j ["id"] = d.id;
	j ["created"] = d.created;
	j ["created_by"] = d.createdBy;
	j ["created_on"] = d.createdOn;
	if (d.pictureUrl != "") j ["picture_url"] = d.pictureUrl;
}