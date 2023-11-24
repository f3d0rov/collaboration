
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
	pe.created = j.at("created").get <bool>();
	if (pe.created) {
		pe.entityId = j.at("entity_id").get <int>();
		if (j.contains ("name")) pe.name = j.at("name").get <std::string>();
	} else {
		pe.name = j.at("name").get <std::string>();
		pe.updateId();
	}
}


UpdateQueryElement <ParticipantEntity>::UpdateQueryElement (std::string jsonElementName, std::string sqlColName, std::string sqlTableName):
UpdateQueryElement <int> (jsonElementName, sqlColName, sqlTableName) {

}

UQEI_ptr UpdateQueryElement <ParticipantEntity>::make (std::string jsonElementName, std::string sqlColName, std::string sqlTableName) {
	return std::make_shared <UpdateQueryElement <ParticipantEntity>> (jsonElementName, sqlColName, sqlTableName);
}

std::string UpdateQueryElement <ParticipantEntity>::getWrappedValue (const nlohmann::json &value, OwnedConnection &w) {
	ParticipantEntity pe;
	from_json (value, pe);
	return std::to_string (pe.entityId);
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
	j["order"] = itd.order;
}




EventType::~EventType () = default;


std::vector <ParticipantEntity> EventType::getParticipantsFromJson (nlohmann::json &data) {
	return this->getParameter <std::vector <ParticipantEntity>> ("participants", data);
}

void EventType::addParticipants (const int eventId, const std::vector <ParticipantEntity> &participants, OwnedConnection &work) {
	if (participants.size() == 0) return;
	std::string addParticipantsQuery = "INSERT INTO participation (event_id, entity_id) VALUES ";
	bool notFirst = false;
	std::string eventIdStr = std::to_string (eventId);
	for (const auto &i: participants) {
		if (notFirst) addParticipantsQuery += ",";
		addParticipantsQuery += std::string("(") + eventIdStr + "," + std::to_string (i.entityId) + ")";
		notFirst = true;
	}
	addParticipantsQuery += ";";
	auto res = work.exec (addParticipantsQuery);
	if (res.affected_rows() != participants.size()) {
		logger << "EventType::addParticipants: res.affected_rows() != participants.size()" << std::endl;
	}
}

std::vector <ParticipantEntity> EventType::getParticipantsForEvent (int eventId, OwnedConnection &work) {
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

nlohmann::json EventType::formGetEventResponse (OwnedConnection &work, int eventId, std::string desc, int sortIndex, std::string startDate, nlohmann::json &data) {
	return nlohmann::json {
		{"id", eventId},
		{"type", this->getTypeName()},
		{"sort_index", sortIndex},

		{"title", this->getTitleFormat()},
		{"description", desc},
		{"start_date", startDate},

		{"data", data},
		{"participants", this->getParticipantsForEvent (eventId, work)}
	};
}

nlohmann::json EventType::formGetEventResponse (OwnedConnection &work, int eventId, std::string desc, int sortIndex, std::string startDate, std::string endDate, nlohmann::json &data) {
	auto resp = formGetEventResponse (work, eventId, desc, sortIndex, startDate, data);
	resp ["end_date"] = endDate;
	return resp;
}


void EventType::ensureParticipantion (OwnedConnection &work, int eventId, ParticipantEntity &pe) {
	pe.updateId ();
	std::string query = "INSERT INTO participation (event_id, entity_id) VALUES ("s
		+ std::to_string (eventId) + ","
		+ std::to_string (pe.entityId) + ") ON CONFLICT DO NOTHING;";
	work.exec (query);
}

void EventType::updateParticipants (OwnedConnection &work, int eventId, std::vector <ParticipantEntity> pes) {
	std::set <int> mustHave;

	for (auto &i: pes) {
		mustHave.insert (i.entityId);
		this->ensureParticipantion (work, eventId, i);
	}

	std::string checkParticipantsQuery = "SELECT entity_id FROM participation WHERE event_id="s + std::to_string (eventId) + ";";
	auto allParticipants = work.exec (checkParticipantsQuery);

	std::string removeExceessParticipantsQuery = "DELETE FROM participation WHERE event_id="s + std::to_string (eventId)
		+ "AND entity_id IN (";
	bool foundSome = false;

	for (int i = 0; i < allParticipants.size(); i++) {
		auto row = allParticipants [i];
		int entityId = row.at (0).as <int>();
		if (mustHave.contains (entityId)) continue;
		removeExceessParticipantsQuery += (foundSome ? ","s : ""s) + std::to_string (entityId);
		foundSome = true;
	}

	if (foundSome) {
		removeExceessParticipantsQuery += ");";
		logger << "EventType::updateParticipants removeExceessParticipantsQuery: " << removeExceessParticipantsQuery << std::endl;
		work.exec (removeExceessParticipantsQuery);
	}
}


nlohmann::json EventType::getEventDescriptor () {
	auto inputs = this->getInputs ();
	for (int i = 0; i < inputs.size(); i++) {
		inputs[i].order = i;
	}
	return nlohmann::json{
		{"type_display_name", this->getDisplayName()},
		{"applicable", this->getApplicableEntityTypes()},
		{"inputs", inputs}
	};
}


void EventType::deleteEvent (int id) {
	std::string deleteEventQuery = std::string("DELETE FROM events WHERE id=") + std::to_string(id) + ";";
	auto conn = database.connect();
	
	auto res = conn.exec (deleteEventQuery);
	if (res.affected_rows() > 1) throw std::logic_error ("EventType::deleteEvent: res.affected_rows() > 1");
	if (res.affected_rows() == 0) throw UserSideEventException ("Не существует события с заданным id");
	conn.commit();
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

void EventManager::addUserEventContribution (int userId, int eventId) {
	std::string addContributionQuery = std::string () +
		"INSERT INTO user_event_contributions (user_id, contributed_on, event_id) "
		"VALUES (" + std::to_string (userId) + ",CURRENT_DATE," + std::to_string (eventId) + ")";
	
	auto conn = database.connect();

	conn.exec (addContributionQuery);
	conn.commit ();
}


void EventManager::registerEventType (std::shared_ptr <EventType> et) {
	std::string typeName = et->getTypeName();
	if (this->_types.contains (typeName)) {
		throw std::logic_error (
			std::string("Повтор имени типа события: '") + typeName + "'"
		);
	}
	this->_types [typeName] = et;
	logger << "В EventManager зарегистрирован тип события '" + typeName + "'" << std::endl;
}

int EventManager::size() const {
	return this->_types.size();
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

	auto conn = database.connect();\

	auto res = conn.exec (getEventTypeByIdQuery);
	if (res.size() == 0) throw UserSideEventException ("Нет события с выбранным id");

	std::string typeName = res[0][0].as <std::string>();
	return this->getEventTypeByName (typeName);
}

int EventManager::createEvent (nlohmann::json &data, int byUser) {
	int id = this->getEventTypeFromJson (data)->createEvent (data);
	this->addUserEventContribution (byUser, id);
	return id;
}


nlohmann::json EventManager::getEvent (int eventId) {
	return this->getEventTypeById (eventId)->getEvent (eventId);
}


nlohmann::json EventManager::getEventsForEntity (int entityId) {
	std::string getEventListQuery = std::string()
		+ "SELECT id FROM events INNER JOIN participation ON participation.event_id=events.id WHERE entity_id="
		+ std::to_string (entityId) + ";";

	pqxx::result result;

	{ // {} used as a safety measure to avoid keeping `conn` and `work` in scope after calling `conn.release()`
		auto conn = database.connect ();
		result = conn.exec (getEventListQuery);
	}

	std::vector <int> eventIds;

	for (int i = 0; i < result.size(); i++) {
		eventIds.push_back (result[i][0].as <int>());
	}

	std::vector <nlohmann::json> events;
	events.reserve (eventIds.size());

	for (const auto &i: eventIds) {
		events.push_back (this->getEvent (i));
	}

	return nlohmann::json {{"events", events}};
}

int EventManager::updateEvent (nlohmann::json &data, int byUser) {
	auto id = this->getEventTypeFromJson (data)->updateEvent (data);
	this->addUserEventContribution (byUser, id);
	return id;
}

void EventManager::deleteEvent (int eventId, int byUser) {
	this->getEventTypeById (eventId)->deleteEvent (eventId);
	this->addUserEventContribution (byUser, eventId);
}

nlohmann::json EventManager::getAvailableEventDescriptors () {
	nlohmann::json result = nlohmann::json::object();
	for (const auto &i: this->_types) {
		auto et = i.second;
		result [i.first] = et->getEventDescriptor();
	}
	return result;
}

std::vector <EventManager::EventReportReason> EventManager::getEventReportReasons () {
	std::string getEventReportReasonsQuery = "SELECT id, name FROM event_report_reasons;";
	auto conn = database.connect();
	auto result = conn.exec (getEventReportReasonsQuery);
	
	std::vector <EventManager::EventReportReason> eventReportReasons;

	for (int i = 0; i < result.size(); i++) {
		EventManager::EventReportReason reason;
		reason.id = result[i].at ("id").as <int>();
		reason.name = result[i].at ("name").as <std::string>();
		eventReportReasons.push_back (reason);
	}

	return eventReportReasons;
}

void to_json (nlohmann::json &j, const EventManager::EventReportReason &evrr) {
	j["id"] = evrr.id;
	j["name"] = evrr.name;
}

void to_json (nlohmann::json &j, const EventManager::EventReportCount &evrc) {
	j["id"] = evrc.reportTypeId;
	j["count"] = evrc.count;
}

void to_json (nlohmann::json &j, const EventManager::EventReportData &evrd) {
	j["event_id"] = evrd.eventId;
	j["reports"] = evrd.statistics;
}

void EventManager::reportEvent (int id, int reasonId, int byUser) {
	std::string reportEventQuery = std::string() +
		"INSERT INTO event_reports (event_id, reported_by, reason_id) VALUES ("
		+ std::to_string (id) + ","
		+ std::to_string (byUser) + ","
		+ std::to_string (reasonId) + ");";
	try {
		auto conn = database.connect();
		conn.exec (reportEventQuery);
		conn.commit();
	} catch (pqxx::sql_error &e) {
		logger << "EventManager::reportEvent: " << e.what() << std::endl;
		throw UserSideEventException ("No such id"); // ?
	}
}
