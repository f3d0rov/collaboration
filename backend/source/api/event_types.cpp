
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
	// TODO: verify entity type is correct via this->entityTypeApplicable

	std::string addEventQuery = std::string ()
		+ "INSERT INTO events (sort_index, type, description) VALUES ("
		+ /* sort_index */ 	"0,"
		+ /* type */		conn.quote (this->getTypeName()) + ","
		+ /* desc */		conn.quote (data.description)
		+ ") RETURNING id;";
	
	auto res = conn.exec (addEventQuery);
	int eventId = res[0][0].as <int>();


	std::string addEventDataQuery = std::string ()
		+ "INSERT INTO single_entity_related_events (id, entity_id, event_date) VALUES ("
		+ /* id */			std::to_string (eventId) + ","
		+ /* entity_id */	std::to_string (data.entity.entityId) + ","
		+ /* event_date */	conn.quote (data.date)
		+ ");";
	
	conn.exec (addEventDataQuery);

	this->addParticipants (eventId, participants, conn);
	this->ensureParticipantion (conn, eventId, data.entity); // Related entity considered a participant

	conn.commit();
	
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
	
	auto res = conn.exec (getEventDataQuery);
	if (res.size() == 0) throw UserSideEventException ("Не существует события с заданным id");
	auto row = res[0];

	SingleEntityRelatedEventType::Data data;
	data.date = row["event_date"].as <std::string>();
	data.entity.created = !(row ["awaits_creation"].as <bool>());
	data.entity.entityId = row ["entity_id"].as <int>();
	data.entity.name = row ["name"].as <std::string>();
	data.description = row ["description"].as <std::string> ();

	nlohmann::json dataJson;
	to_json (dataJson, data);

	return this->formGetEventResponse (conn, id, "{description}", row["sort_index"].as <int>(), data.date, dataJson);
}

int SingleEntityRelatedEventType::updateEvent (nlohmann::json &data) {
	int eventId = this->getParameter<int> ("id", data);
	auto conn = database.connect();

	if (data.contains ("data")) {
		UpdateQueryGenerator qGen (
			{
				UpdateQueryElement <std::string>::make ("event_date", "event_date", "single_entity_related_events"),
				UpdateQueryElement <ParticipantEntity>::make ("entity", "entity_id", "single_entity_related_events"),
				UpdateQueryElement <std::string>::make ("description", "description", "events")
			},
			{
				UpdateQueryGenerator::TableColumnValue {"events", "id", std::to_string (eventId)},
				UpdateQueryGenerator::TableColumnValue {"single_entity_related_events", "id", std::to_string (eventId)}
			}
		);

		std::string updateQuery = qGen.queryFor (data.at ("data"), conn);
		conn.exec (updateQuery);
	}

	if (data.contains ("participants")) {
		std::vector <ParticipantEntity> pes;
		pes = this->getParameter <std::vector <ParticipantEntity>> ("participants", data);
		int mustHaveEntityId = this->getRelatedEntityForEvent (conn, eventId);
		pes.push_back (ParticipantEntity{true, mustHaveEntityId, ""});
		this->updateParticipants (conn, eventId, pes);
	}

	conn.commit();
	return eventId;
}


int SingleEntityRelatedEventType::getRelatedEntityForEvent (OwnedConnection &work, int eventId) {
	std::string query = "SELECT entity_id FROM single_entity_related_events WHERE id="s + std::to_string (eventId) + ";";
	auto result = work.exec (query);
	if (result.size() == 0)
		throw std::runtime_error ("SingleEntityRelatedEventType::getRelatedEntityForEvent: нет события с id="s + std::to_string (eventId));
	else {
		return result.at(0).at(0).as <int> ();
	}
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
	int albumIndex = rawData["data"].contains ("album_index") ? getParameter <int> ("album_index", rawData ["data"]) : 0;
	int albumId = rawData["data"].contains ("album") ? getParameter <int> ("album", rawData ["data"]) : 0;

	auto conn = database.connect();

	std::string createEventQuery = std::string()
		+ "INSERT INTO events (type, description) VALUES ("
		+ /* type */	conn.quote (this->getTypeName()) + ","
		+ /* desc */	conn.quote (data.description) + ") RETURNING id;";
	auto res = conn.exec1 (createEventQuery);
	int eventId = res[0].as <int>();

	this->addParticipants (eventId, participants, conn);

	std::string addSongQuery = std::string()
		+ "INSERT INTO songs (title, author, release, release_date, album, album_index) VALUES ("
		+ /* title */ 	conn.quote (data.song) + ","
		+ /* author */	std::to_string (data.author.entityId) + ","
		+ /* release */	std::to_string (eventId) + ","
		+ /* date */ 	conn.quote (data.date) + ","
		+ /* album */	((albumId != 0) ? std::to_string (albumId) : "NULL") + ","
		+ /* alb_indx */std::to_string (albumIndex)
		+ ");";
	conn.exec (addSongQuery);

	conn.commit();

	this->indexSingle (eventId, albumId);

	return eventId;
}

nlohmann::json SinglePublicationEventType::getEvent (int id) {
	std::string getEventQuery = std::string (
		"SELECT events.sort_index, events.description,"
		"songs.title, songs.release_date,"
		"songs.author, entities.name, entities.awaits_creation, "
		"songs.album, albums.author AS album_author "
		"FROM events INNER JOIN songs ON events.id = songs.release "
		"INNER JOIN entities ON songs.author = entities.id "
		"LEFT OUTER JOIN albums ON albums.id=songs.album "
		"WHERE events.id=") + std::to_string (id) + ";";

	auto conn = database.connect();


	auto result = conn.exec (getEventQuery);
	if (result.size() == 0) throw UserSideEventException ("Не существует события с заданным id");
	auto row = result[0];

	SinglePublicationEventType::Data data;
	data.author.entityId = row.at ("author").as <int>();
	data.author.created = !(row.at ("awaits_creation").as <bool>());
	data.author.name = row.at ("name").as <std::string>();
	data.song = row.at ("title").as <std::string>();
	data.description = row.at ("description").as <std::string> ();
	data.date = row.at ("release_date").as <std::string> ();

	nlohmann::json dataJson;
	to_json (dataJson, data);

	auto resp = this->formGetEventResponse (conn, id, "{description}", row.at ("sort_index").as <int>(), data.date, dataJson);

	if (row.at ("album_author").is_null() == false) {
		resp ["hide_for_entities"] = nlohmann::json::array();
		resp ["hide_for_entities"].push_back (row.at ("album_author").as <int>());
	}

	return resp;
}

int SinglePublicationEventType::updateEvent (nlohmann::json &data) {
	int eventId = this->getParameter<int> ("id", data);
	auto conn = database.connect();

	if (data.contains ("data")) {
		UpdateQueryGenerator qGen (
			{
				UpdateQueryElement <std::string>::make ("song", "title", "songs"),
				UpdateQueryElement <std::string>::make ("date", "release_date", "songs"),
				UpdateQueryElement <std::string>::make ("description", "description", "events"),
				UpdateQueryElement <ParticipantEntity>::make ("author", "author", "songs"),
				UpdateQueryElement <int>::make ("album_index", "album_index", "songs")
			},
			{
				UpdateQueryGenerator::TableColumnValue {"events", "id", std::to_string (eventId)},
				UpdateQueryGenerator::TableColumnValue {"songs", "release", std::to_string (eventId)}
			}
		);

		std::string updateQuery = qGen.queryFor (data.at ("data"), conn);
		conn.exec (updateQuery);
	}
	
	if (data.contains ("participants")) {
		std::vector <ParticipantEntity> pes;
		pes = this->getParameter <std::vector <ParticipantEntity>> ("participants", data);
		int mustHaveEntityId = this->getAuthorForEvent (conn, eventId);
		pes.push_back (ParticipantEntity{true, mustHaveEntityId, ""});
		this->updateParticipants (conn, eventId, pes);
	}

	conn.commit();

	this->indexSingle (eventId, 0);

	return eventId;
}

int SinglePublicationEventType::getAuthorForEvent (OwnedConnection &work, int eventId) {
	std::string query = "SELECT author FROM songs WHERE release="s + std::to_string (eventId) + ";";
	auto result = work.exec (query);
	if (result.size() == 0)
		throw std::runtime_error ("SinglePublicationEventType::getAuthorForEvent: нет события с id="s + std::to_string (eventId));
	else {
		return result.at(0).at(0).as <int> ();
	}
}

/* static */ int SinglePublicationEventType::getEventIdForSong (int songId) {
	auto conn = database.connect();
	std::string query = "SELECT release FROM songs WHERE id=" + std::to_string (songId) + ";";
	auto row = conn.exec1 (query);
	return row.at (0).as <int>();
}


void SinglePublicationEventType::indexSingle (int eventId, int albumId) {
	int index;

	auto songData = this->getEvent (eventId);
	auto data = songData.at ("data").get <SinglePublicationEventType::Data>();
	auto participants = songData.at ("participants").get <std::vector <ParticipantEntity>>();


	if (this->eventHasIndex (eventId)) {
		index = this->clearIndex (eventId, data.author.name + " - " + data.song, data.description);
	} else {
		std::string url = (albumId != 0) ? ("/a?id=" + std::to_string (albumId)) : getEntityUrl (data.author.entityId);
		index = this->indexThis (eventId, data.author.name + " - " + data.song, data.description, url);
	}

	this->indexKeyword (index, data.song, SEARCH_VALUE_TITLE);
	this->indexKeyword (index, data.author.name, SEARCH_VALUE_AUTHOR);
	this->indexKeyword (index, data.description, SEARCH_VALUE_DESCRIPTION);

	for (const auto &i: participants) {
		this->indexKeyword (index, i.name, SEARCH_VALUE_PARTICIPANT);
	}
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
	j ["description"] = d.description;
	j ["song"] = d.song;
}

