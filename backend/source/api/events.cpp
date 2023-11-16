
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
	}
}




InputTypeDescriptor::InputTypeDescriptor () {

}

InputTypeDescriptor::InputTypeDescriptor (std::string id, std::string prompt, std::string type, bool optional):
id (id), prompt (prompt), type (type), optional (optional) {

}

InputTypeDescriptor::operator nlohmann::json () {
	return nlohmann::json{
		{"id", this->id},
		{"prompt", this->prompt},
		{"type", this->type},
		{"optional", this->optional}
	};
}




std::vector <ParticipantEntity> EventType::getParticipants (nlohmann::json &data) {
	auto participants =  this->getParameter <std::vector <ParticipantEntity>> ("participants", data);
	for (auto &i: participants) {
		i.updateId ();
	}
}

nlohmann::json EventType::getEventDescriptor () {
	return nlohmann::json{
		{"applicable", this->getApplicableEntityTypes()},
		{"inputs", this->getInputs()}
	};
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
			std::string("Duplicate event type name: '") + typeName + "'"
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




std::string BandFoundationEventType::getTypeName () const {
	return "band_foundation";
}

std::vector <InputTypeDescriptor> BandFoundationEventType::getInputs () const {
	return {
		InputTypeDescriptor ("date", "Дата основания", "date"),
		InputTypeDescriptor ("band", "Группа", "band"),
		InputTypeDescriptor ("description", "Описание", "textarea", true)
	};
}

std::vector <std::string> BandFoundationEventType::getApplicableEntityTypes () const {
	return { "person" };
}

int BandFoundationEventType::createEvent (nlohmann::json &rawData) {
	auto participants = this->getParticipants (rawData);
	BandFoundationEventType::Data data = this->getParameter <BandFoundationEventType::Data> ("data", rawData);

	auto conn = database.connect();
	pqxx::work work (*conn.conn);

	// TODO: verify entity type == band

	std::string addEventQuery = std::string ()
		+ "INSERT INTO events (sort_index, type, description) VALUES ("
		+ /* sort_index */ 	"0,"
		+ /* type */		work.quote (this->getTypeName()) + ","
		+ /* desc */		work.quote (data.description) + ","
		+ ") RETURNING id;";
	
	auto res = work.exec (addEventQuery);
	int eventId = res[0][0].as <int>();


	std::string addEventDataQuery = std::string ()
		+ "INSERT INTO band_foundation_events (id, entity_id, event_date) VALUES ("
		+ /* id */			std::to_string (eventId) + ","
		+ /* entity_id */	std::to_string (data.band.entityId) + ","
		+ /* event_date */	work.quote (data.date)
		+ ");";
	
	work.exec (addEventDataQuery);
	work.commit();
	
	return eventId;
}

nlohmann::json BandFoundationEventType::getEvent (int id) {
	std::string getEventDataQuery = std::string()
		+ "SELECT events.id, sort_index, description,"
			"entity_id, event_date,"
			"name, awaits_creation "
			"FROM events INNER JOIN band_foundation_events ON events.id = band_foundation_events.id "
			"INNER JOIN entities ON band_foundation_events.entity_id = entities.id "
			"WHERE events.id=" + std::to_string (id) + ";";
	
	auto conn = database.connect();
	pqxx::work work (*conn.conn);
	auto res = work.exec (getEventDataQuery);
	if (res.size() == 0) throw UserSideEventException ("Не существует события с заданным id");
	auto row = res[0];

	ParticipantEntity band;
	band.created = !(row ["awaits_creation"].as <bool>());
	band.entityId = row ["entity_id"].as <int>();
	band.name = row ["name"].as <std::string>();

	nlohmann::json result = {
		{"id", row["id"].as <int>()},
		{"type", this->getTypeName()},
		{"sort_index", row["sort_index"].as <std::string>()},
		{"description", row["description"].as <std::string>()},
		{"data", {
			{"band", band},
			{"event_date", row["event_date"].as <std::string>()}
		}}
	};

	return result;
}

void BandFoundationEventType::updateEvent (nlohmann::json &data) {
	// TODO: this
	int eventId = this->getParameter<int> ("id", data);
	std::string changeEventQuery = "UPDATE events SET ";
	bool nFirst = false;
	if (data.contains ("sort_index")) { // TODO: form query using templates like getQueryString <int> ("sort_index", data)
		changeEventQuery += "sort_index=" + std::to_string (this->getParameter<int> ("sort_index", data)) + ",";

	}
	if (data.contains ("description"))
}

void BandFoundationEventType::deleteEvent (int id) {
	std::string deleteEventQuery = std::string("DELETE FROM events WHERE id=") + std::to_string(id) + ";";
	auto conn = database.connect();
	pqxx::work work(*conn.conn);
	auto res = work.exec (deleteEventQuery);
	if (res.affected_rows() > 1) throw std::logic_error ("BandFoundationEventType::deleteEvent: res.affected_rows() > 1");
	if (res.affected_rows() == 0) throw UserSideEventException ("Не существует события с заданным id");
	work.commit();
}


void from_json (const nlohmann::json &j, BandFoundationEventType::Data &d) {
	d.date = j ["date"].get <std::string> ();
	d.description = j ["description"].get <std::string> ();
	d.band = j ["band"].get <ParticipantEntity> ();
	d.band.updateId ();
}

void to_json (nlohmann::json &j, const BandFoundationEventType::Data &d) {
	j ["date"] = d.date;
	j ["description"] = d.description;
	j ["band"] = d.band;
}

