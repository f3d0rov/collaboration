
#include "events.hpp"

#define TYPE "type"


UserSideEventException::UserSideEventException (std::string w):
std::runtime_error (w) {

}




void ParticipantEntity::updateId () {
	if (!created) {
		this->entityId = EntityDataResource::getEntityByName (this->name);
	}
}

void to_json (nlohmann::json &j, const ParticipantEntity &pe) {
	// ? Should entity_id be set independently of pe.created?
	j["created"] = pe.created;
	if (pe.created) {
		j["entity_id"] = pe.entityId;
		if (pe.name != "") j["name"] = pe.name;
	} else {
		j["name"] = pe.name;
	}
}

void from_json (const nlohmann::json &j, ParticipantEntity &pe) {
	pe.created = j["created"].get <bool>();
	if (pe.created) {
		pe.entityId = j["entity_id"].get <int>();
		if (j.contains ("name")) pe.name = j["name"].get <std::string>();
	} else {
		pe.name = j["name"].get <std::string>();
		pe.updateId();
	}
}




InputTypeDescriptor::InputTypeDescriptor () {

}

InputTypeDescriptor::InputTypeDescriptor (std::string id, std::string prompt, std::string type, bool optional):
id (id), prompt (prompt), type (type), optional (optional) {

}

void to_json (nlohmann::json &j, const InputTypeDescriptor &itd) {
	j["id"] = itd.id;
	j["prompt"] = itd.prompt;
	j["type"] = itd.type;
	j["optional"] = itd.optional;
}



template <>
std::string EventType::getUpdateQueryString <ParticipantEntity> (std::string jsonParam, std::string colName, nlohmann::json &data, pqxx::work &work, bool &notFirst) {
	if (data.contains (jsonParam)) {
		ParticipantEntity entity = data [jsonParam].get <ParticipantEntity>();
		notFirst = true;
		return std::string(notFirst ? "," : "") + colName + "=" + std::to_string(entity.entityId);
	}
	return "";
}

std::vector <ParticipantEntity> EventType::getParticipantsFromJson (nlohmann::json &data) {
	return this->getParameter <std::vector <ParticipantEntity>> ("participants", data);
}

void EventType::addParticipants (const int eventId, const std::vector <ParticipantEntity> &participants, pqxx::work &work) {
	if (participants.size() == 0) return;
	std::string addParticipantsQuery = "INSERT INTO participation (event_id, entity_id) VALUES ";
	bool notFirst = false;
	std::string eventIdStr = std::to_string (eventId);
	for (const auto &i: participants) {
		if (notFirst) addParticipantsQuery += ",";
		addParticipantsQuery += std::string("(") + eventIdStr + "," + std::to_string (i.entityId) + ")";
	}
	addParticipantsQuery += ";";
	auto res = work.exec (addParticipantsQuery);
	if (res.affected_rows() != participants.size()) {
		logger << "EventType::addParticipants: res.affected_rows() != participants.size()" << std::endl;
	}
}

std::vector <ParticipantEntity> EventType::getParticipantsForEvent (int eventId, pqxx::work &work) {
	std::string getParticipantsQuery = std::string (
		"SELECT awaits_creation, entities.id, name "
		"FROM participation INNER JOIN entities ON participation.entity_id=entities.id "
		"WHERE event_id="
	) + std::to_string (eventId) + ";";
	auto result = work.exec (getParticipantsQuery);

	std::vector <ParticipantEntity> participants;
	for (int i = 0; i < result.size(); i++) {
		auto row = result[i];
		ParticipantEntity pe;
		pe.created = !(row ["awaits_creation"].as <bool>());
		pe.entityId = row ["id"].as <int>();
		pe.name = row ["name"].as <std::string>();

		participants.push_back (pe);
	}
	
	return participants;
}

void EventType::updateCommonEventData (int eventId, nlohmann::json &data, pqxx::work &work) {
	bool notFirst = false;
	std::string changeEventQuery = "UPDATE events SET ";
	changeEventQuery += this->getUpdateQueryString<int> ("sort_index", "sort_index", data, work, notFirst);
	changeEventQuery += this->getUpdateQueryString<std::string> ("description", "description", data, work, notFirst);
	changeEventQuery += std::string(" WHERE id=") + std::to_string (eventId) + ";";
	if (notFirst /* changing something? */) work.exec (changeEventQuery);

}

nlohmann::json EventType::getEventDescriptor () {
	return nlohmann::json{
		{"type_display_name", this->getDisplayName()},
		{"applicable", this->getApplicableEntityTypes()},
		{"inputs", this->getInputs()}
	};
}


template <>
std::string EventType::wrapType <int> (const int &t, pqxx::work &work) {
	return std::to_string (t);
}

template <>
std::string EventType::wrapType <std::string> (const std::string &s, pqxx::work &work) {
	return work.quote (s);
}


void EventType::deleteEvent (int id) {
	std::string deleteEventQuery = std::string("DELETE FROM events WHERE id=") + std::to_string(id) + ";";
	auto conn = database.connect();
	pqxx::work work(*conn.conn);
	auto res = work.exec (deleteEventQuery);
	if (res.affected_rows() > 1) throw std::logic_error ("EventType::deleteEvent: res.affected_rows() > 1");
	if (res.affected_rows() == 0) throw UserSideEventException ("Не существует события с заданным id");
	work.commit();
}



std::vector <std::string> AllEntitiesEventType::getApplicableEntityTypes () const {
	return {"band", "person"};
}

std::vector <std::string> BandOnlyEventType::getApplicableEntityTypes () const {
	return {"band"};
}

std::vector <std::string> PersonOnlyEventType::getApplicableEntityTypes () const {
	return {"person"};
}




EventManager *EventManager::_obj = nullptr;

EventManager::EventManager () {

}

EventManager &EventManager::getManager () {
	if (EventManager::_obj == nullptr) EventManager::_obj = new EventManager{};
	return *EventManager::_obj;
}

void EventManager::registerEventType (std::shared_ptr <EventType> et) {
	std::string typeName = et->getTypeName();
	if (this->_types.contains (typeName)) {
		throw std::logic_error (
			std::string("Повтор имени типа события: '") + typeName + "'"
		);
	}
	this->_types [typeName] = et;
}

std::shared_ptr <EventType> EventManager::getEventTypeByName (std::string typeName) {
	if (this->_types.contains (typeName) == false) {
		throw UserSideEventException (std::string("Некорректный тип события: '") + typeName + "'");
	}
	return this->_types [typeName];
}


std::shared_ptr <EventType> EventManager::getEventTypeFromJson (nlohmann::json &data) {
	if (data.contains (TYPE) == false) {
		throw UserSideEventException ("Отсутсвует поле 'type' при создании события");
	}

	std::string typeName;
	try {
		typeName = data [TYPE].get <std::string>();
	} catch (nlohmann::json::exception &e) {
		throw UserSideEventException ("Невозможно привести поле 'type' к строке при создании события");
	}

	return this->getEventTypeByName (typeName);
}

std::shared_ptr <EventType> EventManager::getEventTypeById (int eventId) {
	std::string getEventTypeByIdQuery = std::string () +
		"SELECT type FROM events WHERE id=" + std::to_string (eventId) + ";";

	auto conn = database.connect();
	pqxx::work work (*conn.conn);

	auto res = work.exec (getEventTypeByIdQuery);
	if (res.size() == 0) throw UserSideEventException ("Нет события с выбранным id");

	std::string typeName = res[0][0].as <std::string>();
	return this->getEventTypeByName (typeName);
}

int EventManager::createEvent (nlohmann::json &data, int byUser) {
	// TODO: check user creds, add user contrib
	return this->getEventTypeFromJson (data)->createEvent (data);
}


nlohmann::json EventManager::getEvent (int eventId) {
	return this->getEventTypeById (eventId)->getEvent (eventId);
}

void EventManager::updateEvent (nlohmann::json &data, int byUser) {
	// TODO: check user creds, add user contrib
	return this->getEventTypeFromJson (data)->updateEvent (data);
}

void EventManager::deleteEvent (int eventId, int byUser) {
	// TODO: check user creds, add user contrib
	return this->getEventTypeById (eventId)->deleteEvent (eventId);
}

nlohmann::json EventManager::getAvailableEventDescriptors () {
	nlohmann::json result;
	for (auto i: this->_types) {
		auto et = i.second;
		result [i.first] = i.second->getEventDescriptor();
	}
	return result;
}

