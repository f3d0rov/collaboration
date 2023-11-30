
#include "albums.hpp"


/* static */ std::unique_ptr <AlbumManager> AlbumManager::_manager;


AlbumManager::AlbumManager () {

}

/* static */ AlbumManager &AlbumManager::get () {
	if (!AlbumManager::_manager) AlbumManager::_manager = std::unique_ptr <AlbumManager> (new AlbumManager{});
	return *AlbumManager::_manager;
}


std::vector <nlohmann::json> AlbumManager::getSongsForAlbum (int albumId) {
	std::string getSongsQuery =
		"SELECT songs.id, songs.title, songs.author, entities.name, entities.awaits_creation, songs.album_index, release "
		"FROM songs INNER JOIN entities ON songs.author=entities.id WHERE album=" + std::to_string (albumId) + ";";

	auto conn = database.connect();
	auto results = conn.exec (getSongsQuery);
	if (results.size() == 0) return {};

	std::vector <nlohmann::json> songsData;

	for (int i = 0; i < results.size(); ++i) {
		pqxx::row row = results[i];
		nlohmann::json song;
		ParticipantEntity author;
		song ["id"] = row.at ("id").as <int> ();
		song ["song"] = row.at ("title").as <std::string>();

		author.entityId = row.at ("author").as <int> ();
		author.name = row.at ("name").as <std::string> ();
		author.created = !(row.at ("awaits_creation").as <bool> ());
		song ["author"] = author;
		song ["album_index"] = row.at ("album_index").as <int> ();

		if (row.at ("release").is_null() == false) {
			int event = row.at ("release").as <int>();
			std::string getParticipants = 
				"SELECT name, id, awaits_creation FROM participation INNER JOIN entities ON participation.entity_id=entities.id "
				"WHERE event_id=" + std::to_string (event) + ";";
			auto participants = conn.exec (getParticipants);

			std::vector <ParticipantEntity> participantArray;
			for (int i = 0; i < participants.size(); i++) {
				ParticipantEntity pe;
				pqxx::row peRow = participants.at (i);
				pe.name = peRow.at (0).as <std::string>();
				pe.entityId = peRow.at (1).as <int>();
				pe.created = !peRow.at (2).as <bool>();
				participantArray.push_back (pe);
			}
			song ["participants"] = participantArray;
		}

		songsData.push_back (song);
	}

	return songsData;
}


nlohmann::json AlbumManager::getAlbumData (int albumId) {
	auto &eventManager = EventManager::getManager();
	auto base = eventManager.getEvent (albumId);

	// TODO: get album songs, return album songs
	base ["songs"] = this->getSongsForAlbum (albumId);
	return base;
}

int AlbumManager::getAlbumPictureResourceId (int id) {
	std::string query = "SELECT picture FROM albums WHERE id="s + std::to_string (id) + ";";
	auto conn = database.connect ();
	auto result = conn.exec (query);
	if (result.size() == 0) return -1;
	return result.at (0).at (0).as <int>();
}




std::string AlbumEventType::getTypeName () const {
	return "album";
}

std::string AlbumEventType::getDisplayName () const {
	return "Альбом";
}

std::string AlbumEventType::getTitleFormat () const {
	return "Альбом {author} - {album_link}";
}

std::vector <InputTypeDescriptor> AlbumEventType::getInputs () const {
	return {
		InputTypeDescriptor {"author", "", "current_entity"},
		InputTypeDescriptor {"date", "Дата публикации", "date"},
		InputTypeDescriptor {"album", "Имя альбома", "text"},
		InputTypeDescriptor {"description", "Описание", "textarea", true}
	};
}

int AlbumEventType::createEvent (nlohmann::json &rawData) {
	auto participants = this->getParticipantsFromJson (rawData);
	auto data = getParameter <AlbumEventType::Data> ("data", rawData);
	
	auto conn = database.connect();

	std::string createEventQuery = std::string()
		+ "INSERT INTO events (type, description) VALUES ("
		+ /* type */	conn.quote (this->getTypeName()) + ","
		+ /* desc */	conn.quote (data.description) + ") RETURNING id;";
	auto res = conn.exec1 (createEventQuery);
	int eventId = res[0].as <int>();

	this->addParticipants (eventId, participants, conn);

	auto &mgr = UploadedResourcesManager::get();
	auto picRes = mgr.createUploadableResource (false);

	std::string addAlbumQuery = std::string()
		+ "INSERT INTO albums (id, title, author, release_date, picture) VALUES ("
		+ /* id */	std::to_string (eventId) + ","
		+ /* title */ 	conn.quote (data.album) + ","
		+ /* author */	std::to_string (data.author.entityId) + ","
		+ /* date */ 	conn.quote (data.date) + ","
		+ /* pic */		std::to_string (picRes.resourceId) 
		+ ");";
	conn.exec (addAlbumQuery);

	conn.commit();
	return eventId;
}

nlohmann::json AlbumEventType::getEvent (int id) {
	std::string getEventQuery = std::string (
		"SELECT sort_index, events.description,"
		"albums.title, release_date,"
		"author, entities.name, awaits_creation "
		"FROM events INNER JOIN albums ON events.id = albums.id "
		"INNER JOIN entities ON albums.author = entities.id "
		"WHERE events.id=") + std::to_string (id) + ";";

	auto conn = database.connect();

	auto result = conn.exec (getEventQuery);
	if (result.size() == 0) throw UserSideEventException ("Не существует события с заданным id");
	auto row = result[0];

	AlbumEventType::Data data;
	data.author.entityId = row.at ("author").as <int>();
	data.author.created = !(row.at ("awaits_creation").as <bool>());
	data.author.name = row.at ("name").as <std::string>();
	data.album = row.at ("title").as <std::string>();
	data.description = row.at ("description").as <std::string> ();
	data.date = row.at ("release_date").as <std::string> ();

	nlohmann::json dataJson;
	to_json (dataJson, data);
	dataJson ["album_link"] = nlohmann::json {
		{"url", "/a?id="s + std::to_string (id)},
		{"name", data.album }
	};

	return this->formGetEventResponse (conn, id, "{description}", row.at ("sort_index").as <int>(), data.date, dataJson);
}

int AlbumEventType::updateEvent (nlohmann::json &data) {
	int eventId = this->getParameter<int> ("id", data);
	auto conn = database.connect();

	if (data.contains ("data")) {
		UpdateQueryGenerator qGen (
			{
				UpdateQueryElement <std::string>::make ("album", "title", "albums"),
				UpdateQueryElement <std::string>::make ("date", "release_date", "albums"),
				UpdateQueryElement <std::string>::make ("description", "description", "events"),
				UpdateQueryElement <ParticipantEntity>::make ("author", "author", "albums")
			},
			{
				UpdateQueryGenerator::TableColumnValue {"events", "id", std::to_string (eventId)},
				UpdateQueryGenerator::TableColumnValue {"albums", "id", std::to_string (eventId)}
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
	return eventId;
}

int AlbumEventType::getAuthorForEvent (OwnedConnection &work, int eventId) {
	std::string query = "SELECT author FROM albums WHERE id="s + std::to_string (eventId) + ";";
	auto result = work.exec (query);
	if (result.size() == 0)
		throw std::runtime_error ("SinglePublicationEventType::getAuthorForEvent: нет события с id="s + std::to_string (eventId));
	else {
		return result.at(0).at(0).as <int> ();
	}
}

void from_json (const nlohmann::json &j, AlbumEventType::Data &d) {
	d.author = j.at ("author").get <ParticipantEntity>();
	d.date = j.at ("date").get <std::string>();
	d.description = j.at ("description").get <std::string>();
	d.album = j.at ("album").get <std::string>();
}

void to_json (nlohmann::json &j, const AlbumEventType::Data &d) {
	j ["author"] = d.author;
	j ["date"] = d.date;
	j ["description"] = d.description;
	j ["album"] = d.album;
}




AlbumDataResource::AlbumDataResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {
 
}

ApiResponsePtr AlbumDataResource::processRequest (RequestData &rd, nlohmann::json body) {
	assertMethod (rd, "POST");
	AlbumManager &mgr = AlbumManager::get();
	int albumId = getParameter <int> ("id", body);
	return makeApiResponse (mgr.getAlbumData (albumId));
}




RequestAlbumImageChangeResource::RequestAlbumImageChangeResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

ApiResponsePtr RequestAlbumImageChangeResource::processRequest (RequestData &rd, nlohmann::json body) {
	assertMethod (rd, "POST");
	auto &userMgr = UserManager::get();
	if (!userMgr.isLoggedIn (rd.getCookie (SESSION_ID))) return makeApiResponse (401);

	int id = getParameter <int> ("id", body);
	auto &albumMgr = AlbumManager::get();
	int resourceId = albumMgr.getAlbumPictureResourceId (id);

	auto &uplMgr = UploadedResourcesManager::get();
	std::string uploadId = uplMgr.generateUploadIdForResource (resourceId);
	std::string uploadUrl = uplMgr.getUploadUrl (uploadId);

	return makeApiResponse (nlohmann::json {{"url", uploadUrl}}, 200);
}