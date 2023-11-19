
#include "event_types.hpp"


void from_json (const nlohmann::json &j, SingleEntityRelatedEventType::Data &d) {
	d.date = j ["event_date"].get <std::string> ();
	d.description = j ["description"].get <std::string> ();
	d.entity = j ["entity"].get <ParticipantEntity> ();
	// d.band.updateId ();
}

void to_json (nlohmann::json &j, const SingleEntityRelatedEventType::Data &d) {
	j ["event_date"] = d.date;
	j ["description"] = d.description;
	j ["entity"] = d.entity;
}

/***************************************
 * SINGLE ENTITY RELATED EVENT TYPE
***************************************/


std::vector <InputTypeDescriptor> SingleEntityRelatedEventType::getInputs () const {
	return {
		InputTypeDescriptor ("event_date", this->getDatePrompt(), "date"),
		InputTypeDescriptor ("entity", this->getRelatedEntityPromptString(), this->getRelatedEntityType()),
		InputTypeDescriptor ("description", "Описание", "textarea", true)
	};
}

int SingleEntityRelatedEventType::createEvent (nlohmann::json &rawData) {
	auto participants = this->getParticipantsFromJson (rawData);
	SingleEntityRelatedEventType::Data data = this->getParameter <SingleEntityRelatedEventType::Data> ("data", rawData);

	auto conn = database.connect();
	pqxx::work work (*conn.conn);

	// TODO: verify entity type is correct via this->entityTypeApplicable

	std::string addEventQuery = std::string ()
		+ "INSERT INTO events (sort_index, type, description) VALUES ("
		+ /* sort_index */ 	"0,"
		+ /* type */		work.quote (this->getTypeName()) + ","
		+ /* desc */		work.quote (data.description)
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

	this->addParticipants (eventId, participants, work);

	work.commit();
	
	return eventId;
}

nlohmann::json SingleEntityRelatedEventType::getEvent (int id) {
	std::string getEventDataQuery = std::string()
		+ "SELECT sort_index, events.description,"
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

	SingleEntityRelatedEventType::Data data;
	data.date = row["event_date"].as <std::string>();
	data.entity.created = !(row ["awaits_creation"].as <bool>());
	data.entity.entityId = row ["entity_id"].as <int>();
	data.entity.name = row ["name"].as <std::string>();

	nlohmann::json dataJson;
	to_json (dataJson, data);

	return this->formGetEventResponse (work, id, row["description"].as <std::string>(), row["sort_index"].as <int>(), dataJson);
}

int SingleEntityRelatedEventType::updateEvent (nlohmann::json &data) {
	int eventId = this->getParameter<int> ("id", data);
	auto conn = database.connect();
	pqxx::work work (*conn.conn);

	this->updateCommonEventData (eventId, data, work);

	if (data.contains ("data")) {
		auto dataObj = data ["data"];
		bool notFirst = false;
		std::string changeEventQuery = "UPDATE single_entity_related_events SET ";
		changeEventQuery += this->getUpdateQueryString <std::string> ("event_date", "event_date", dataObj, work, notFirst);
		changeEventQuery += this->getUpdateQueryString <ParticipantEntity> ("entity", "entity_id", dataObj, work, notFirst);
		changeEventQuery += std::string(" WHERE id=") + std::to_string (eventId) + ";";
		if (notFirst) work.exec (changeEventQuery);
	}

	work.commit();
	return eventId;
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

std::string BandLeaveEventType::getRelatedEntityType () const {
	return "band";
}

std::string BandLeaveEventType::getRelatedEntityPromptString () const {
	return "Группа";
}

std::string BandLeaveEventType::getDatePrompt () const {
	return "Дата ухода";
}




/***************************************
 * SINGLE PUBLICATION EVENT TYPE
***************************************/

std::string SinglePublicationEventType::getTypeName () const {
	return "ep";
}

std::string SinglePublicationEventType::getDisplayName () const {
	return "Сингл";
}

std::string SinglePublicationEventType::getTitleFormat () const  {
	return "Сингл {author} - {song}";
}

std::vector <InputTypeDescriptor> SinglePublicationEventType::getInputs () const {
	return {
		InputTypeDescriptor {"author", "", "current_entity"},
		InputTypeDescriptor {"date", "Дата публикации", "date"},
		InputTypeDescriptor {"song", "Имя сингла", "text"},
		InputTypeDescriptor {"description", "Описание", "textarea", true}
	};
}

int SinglePublicationEventType::createEvent (nlohmann::json &rawData) {
	auto participants = this->getParticipantsFromJson (rawData);
	SinglePublicationEventType::Data data = this->getParameter <SinglePublicationEventType::Data> ("data", rawData);

	auto conn = database.connect();
	pqxx::work work (*conn.conn);

	std::string createEventQuery = std::string()
		+ "INSERT INTO events (type, description) VALUES ("
		+ /* type */	work.quote (this->getTypeName()) + ","
		+ /* desc */	work.quote (data.description) + ") RETURNING id;";
	auto res = work.exec1 (createEventQuery);
	int eventId = res[0].as <int>();

	this->addParticipants (eventId, participants, work);

	std::string addSongQuery = std::string()
		+ "INSERT INTO songs (title, author, release, release_date) VALUES ("
		+ /* title */ 	work.quote (data.song) + ","
		+ /* author */	std::to_string (data.author.entityId) + ","
		+ /* release */	std::to_string (eventId) + ","
		+ /* date */ 	work.quote (data.date)
		+ ");";
	work.exec (addSongQuery);

	work.commit();
	return eventId;
}

nlohmann::json SinglePublicationEventType::getEvent (int id) {
	std::string getEventQuery = std::string (
		"SELECT sort_index, events.description,"
		"songs.title, release_date,"
		"author, entities.name, awaits_creation "
		"FROM events INNER JOIN songs ON events.id = songs.release "
		"INNER JOIN entities ON songs.author = entities.id "
		"WHERE events.id=") + std::to_string (id) + ";";

	auto conn = database.connect();
	pqxx::work work (*conn.conn);


	auto result = work.exec (getEventQuery);
	if (result.size() == 0) throw UserSideEventException ("Не существует события с заданным id");
	auto row = result[0];

	SinglePublicationEventType::Data data;
	data.author.entityId = row ["author"].as <int>();
	data.author.created = !(row ["awaits_creation"].as <bool>());
	data.author.name = row ["name"].as <std::string>();
	
	nlohmann::json dataJson;
	to_json (dataJson, data);

	return this->formGetEventResponse (work, id, row ["description"].as <std::string>(), row ["sort_index"].as <int>(), dataJson);
}

int SinglePublicationEventType::updateEvent (nlohmann::json &data) {
	int eventId = this->getParameter<int> ("id", data);
	auto conn = database.connect();
	pqxx::work work (*conn.conn);

	this->updateCommonEventData (eventId, data, work);

	if (data.contains ("data")) {
		auto dataObj = data ["data"];
		bool notFirst = false;
		std::string changeEventQuery = "UPDATE songs SET ";
		changeEventQuery += this->getUpdateQueryString <std::string> ("song", "title", dataObj, work, notFirst);
		changeEventQuery += this->getUpdateQueryString <std::string> ("date", "released_on", dataObj, work, notFirst);
		changeEventQuery += this->getUpdateQueryString <ParticipantEntity> ("author", "author", dataObj, work, notFirst);
		changeEventQuery += std::string(" WHERE id=") + std::to_string (eventId) + ";";
		if (notFirst) work.exec (changeEventQuery);
	}

	work.commit();
	return eventId;
}


void from_json (const nlohmann::json &j, SinglePublicationEventType::Data &d) {
	d.author = j.at ("author").get <ParticipantEntity>();
	d.date = j.at ("date").get <std::string>();
	d.description = j.at ("description").get <std::string>();
	d.song = j.at ("song").get <std::string>();
}

void to_json (nlohmann::json &j, const SinglePublicationEventType::Data &d) {
	j ["author"] = d.author;
	j ["date"] = d.date;
	// j ["description"] = d.description;
	j ["song"] = d.song;
}
