
#include "event_types.hpp"


void from_json (const nlohmann::json &j, SingleEntityRelatedEventType::Data &d) {
	d.date = j ["date"].get <std::string> ();
	d.description = j ["description"].get <std::string> ();
	d.entity = j ["entity"].get <ParticipantEntity> ();
	// d.band.updateId ();
}

void to_json (nlohmann::json &j, const SingleEntityRelatedEventType::Data &d) {
	j ["date"] = d.date;
	j ["description"] = d.description;
	j ["entity"] = d.entity;
}

/***************************************
 * SINGLE ENTITY RELATED EVENT TYPE
***************************************/


std::vector <InputTypeDescriptor> SingleEntityRelatedEventType::getInputs () const {
	return {
		InputTypeDescriptor ("date", this->getDatePrompt(), "date"),
		InputTypeDescriptor ("entity", this->getRelatedEntityPromptString(), this->getRelatedEntityType()),
		InputTypeDescriptor ("description", "Описание", "textarea", true)
	};
}

int SingleEntityRelatedEventType::createEvent (nlohmann::json &rawData) {
	auto participants = this->getParticipants (rawData);
	SingleEntityRelatedEventType::Data data = this->getParameter <SingleEntityRelatedEventType::Data> ("data", rawData);

	auto conn = database.connect();
	pqxx::work work (*conn.conn);

	// TODO: verify entity type is correct via this->entityTypeApplicable

	std::string addEventQuery = std::string ()
		+ "INSERT INTO events (sort_index, type, description) VALUES ("
		+ /* sort_index */ 	"0,"
		+ /* type */		work.quote (this->getTypeName()) + ","
		+ /* desc */		work.quote (data.description) + ","
		+ ") RETURNING id;";
	
	auto res = work.exec (addEventQuery);
	int eventId = res[0][0].as <int>();


	std::string addEventDataQuery = std::string ()
		+ "INSERT INTO single_entity_related_events (id, entity_id, event_date) VALUES ("
		+ /* id */			std::to_string (eventId) + ","
		+ /* entity_id */	std::to_string (data.entity.entityId) + ","
		+ /* event_date */	work.quote (data.date)
		+ ");";
	
	work.exec (addEventDataQuery);
	work.commit();
	
	return eventId;
}

nlohmann::json SingleEntityRelatedEventType::getEvent (int id) {
	std::string getEventDataQuery = std::string()
		+ "SELECT events.id, sort_index, description,"
			"entity_id, event_date,"
			"name, awaits_creation "
			"FROM events INNER JOIN single_entity_related_events ON events.id = single_entity_related_events.id "
			"INNER JOIN entities ON single_entity_related_events.entity_id = entities.id "
			"WHERE events.id=" + std::to_string (id) + ";";
	
	auto conn = database.connect();
	pqxx::work work (*conn.conn);
	auto res = work.exec (getEventDataQuery);
	if (res.size() == 0) throw UserSideEventException ("Не существует события с заданным id");
	auto row = res[0];

	ParticipantEntity entity;
	entity.created = !(row ["awaits_creation"].as <bool>());
	entity.entityId = row ["entity_id"].as <int>();
	entity.name = row ["name"].as <std::string>();

	nlohmann::json result = {
		{"id", row["id"].as <int>()},
		{"title", this->getTitleFormat()},
		{"type", this->getTypeName()},
		{"sort_index", row["sort_index"].as <std::string>()},
		{"description", row["description"].as <std::string>()},
		{"data", {
			{"entity", entity},
			{"event_date", row["event_date"].as <std::string>()}
		}}
	};

	return result;
}

void SingleEntityRelatedEventType::updateEvent (nlohmann::json &data) {
	int eventId = this->getParameter<int> ("id", data);
	auto conn = database.connect();
	pqxx::work work (*conn.conn);

	bool notFirst = false;
	std::string changeEventQuery = "UPDATE events SET ";
	changeEventQuery += this->getUpdateQueryString<int> ("sort_index", data, work, notFirst);
	changeEventQuery += this->getUpdateQueryString<std::string> ("description", data, work, notFirst);
	changeEventQuery += std::string(" WHERE id=") + std::to_string (eventId) + ";";
	if (notFirst /* changing something? */) work.exec (changeEventQuery);

	if (data.contains ("data")) {
		auto dataObj = data ["data"];
		notFirst = false;
		changeEventQuery = "UPDATE single_entity_related_events SET ";
		changeEventQuery += this->getUpdateQueryString <std::string> ("event_date", dataObj, work, notFirst);
		if (dataObj.contains ("entity")) {
			ParticipantEntity entity = dataObj ["entity"].get <ParticipantEntity>();
			changeEventQuery += std::string(notFirst ? "," : "") + "entity_id=" + std::to_string(entity.entityId);
			notFirst = true;
		}
		changeEventQuery += std::string(" WHERE id=") + std::to_string (eventId) + ";";
		if (notFirst) work.exec (changeEventQuery);
	}

	work.commit();
}




/***************************************
 * BAND FOUNDATION EVENT TYPE
***************************************/

std::string BandFoundationEventType::getTypeName () const {
	return "band_foundation";
}

std::string BandFoundationEventType::getDisplayName () const {
	return "Основание группы";
}

std::string BandFoundationEventType::getTitleFormat () const {
	return "Основание группы {entity}";
}

std::vector <std::string> BandFoundationEventType::getApplicableEntityTypes () const {
	return { "person" };
}

std::string BandFoundationEventType::getRelatedEntityType () const {
	return "band";
}

std::string BandFoundationEventType::getRelatedEntityPromptString () const {
	return "Группа";
}

std::string BandFoundationEventType::getDatePrompt () const {
	return "Дата основания";
}




/***************************************
 * BAND JOIN EVENT TYPE
***************************************/
std::string BandJoinEventType::getTypeName () const {
	return "band_join";
}

std::string BandJoinEventType::getDisplayName () const {
	return "Присоединение к группе";
}

std::string BandJoinEventType::getTitleFormat () const {
	return "Присоединение к группе {entity}";
}

std::vector <std::string> BandJoinEventType::getApplicableEntityTypes () const {
	return { "person" };
}

std::string BandJoinEventType::getRelatedEntityType () const {
	return "band";
}

std::string BandJoinEventType::getRelatedEntityPromptString () const {
	return "Группа";
}

std::string BandJoinEventType::getDatePrompt () const {
	return "Дата присоединения";
}




/***************************************
 * BAND LEAVE EVENT TYPE
***************************************/

std::string BandLeaveEventType::getTypeName () const {
	return "band_leave";
}

std::string BandLeaveEventType::getDisplayName () const {
	return "Уход из группы";
}

std::string BandLeaveEventType::getTitleFormat () const {
	return "Уход из группы {entity}";
}

std::vector <std::string> BandLeaveEventType::getApplicableEntityTypes () const {
	return { "person" };
}

std::string BandLeaveEventType::getRelatedEntityType () const {
	return "band";
}

std::string BandLeaveEventType::getRelatedEntityPromptString () const {
	return "Группа";
}

std::string BandLeaveEventType::getDatePrompt () const {
	return "Дата ухода";
}

