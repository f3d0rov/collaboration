
#include "albums.hpp"


/* static */ std::unique_ptr <AlbumManager> AlbumManager::_manager;


AlbumManager::AlbumManager () {

}

/* static */ AlbumManager &AlbumManager::get () {
	if (!AlbumManager::_manager) AlbumManager::_manager = std::unique_ptr <AlbumManager> (new AlbumManager{});
	return *AlbumManager::_manager;
}


void AlbumManager::addSongToAlbumByReleaseId (int albumId, int release) {
	std::string query = "UPDATE songs SET album=" + std::to_string (albumId) + " WHERE release=" + std::to_string (release) + ";";
	auto conn = database.connect();
	conn.exec (query);



	conn.commit();
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

std::vector <int> AlbumManager::getSongIdsForAlbum (int albumId) {
	std::string getSongsQuery =
		"SELECT songs.id "
		"FROM songs WHERE album=" + std::to_string (albumId) + ";";

	auto conn = database.connect();
	auto results = conn.exec (getSongsQuery);
	if (results.size() == 0) return {};

	std::vector <int> songIds;
	for (int i = 0; i < results.size(); i++) {
		songIds.push_back (results.at (i).at (0).as <int>());
	}

	return songIds;
}


nlohmann::json AlbumManager::getAlbumData (int albumId) {
	auto &eventManager = EventManager::getManager();
	auto base = eventManager.getEvent (albumId);

	// TODO: get album songs, return album songs
	base ["songs"] = this->getSongsForAlbum (albumId);
	base ["picture"] = this->getAlbumPictureUrl (albumId);
	return base;
}

std::string AlbumManager::albumUrl (int albumId) {
	return "/a?id=" + std::to_string (albumId);
}


int AlbumManager::getAlbumPictureResourceId (int id) {
	std::string query = "SELECT picture FROM albums WHERE id="s + std::to_string (id) + ";";
	auto conn = database.connect ();
	auto result = conn.exec (query);
	if (result.size() == 0) return -1;
	return result.at (0).at (0).as <int>();
}

std::string AlbumManager::getAlbumPictureUrl (int albumId) {
	int resource = this->getAlbumPictureResourceId (albumId);
	auto &mgr = UploadedResourcesManager::get();
	auto url = mgr.getDownloadUrl (resource);
	if (url == "") {
		return "resources/default_picture.svg";
	} else {
		return url;
	}
}

bool AlbumManager::albumIsIndexed (int albumId) {
	std::string getIndexQuery =
		"SELECT search_resource "
		"FROM events "
		"WHERE id="s + std::to_string (albumId) + ";";

	auto conn = database.connect();
	auto row = conn.exec1 (getIndexQuery);
	return row.at (0).is_null() == false;
}

int AlbumManager::getAlbumSearchResIndex (int albumId) {
	std::string getIndexQuery =
		"SELECT search_resource "
		"FROM events "
		"WHERE id="s + std::to_string (albumId) + ";";

	auto conn = database.connect();
	auto row = conn.exec1 (getIndexQuery);
	return row.at (0).as <int> ();
}

int AlbumManager::createAlbumSearchResIndex (int albumId, std::string name, std::string descr) {
	auto &mgr = SearchManager::get();
	int index = mgr.indexNewResource (
		albumId,
		this->albumUrl (albumId),
		name,
		descr,
		"album"
	);
	return index; 
}

int AlbumManager::clearAlbumIndex (int albumId, std::string name, std::string descr) {
	auto &mgr = SearchManager::get();
	int index = this->getAlbumSearchResIndex (albumId);
	mgr.updateResourceData (index, name, descr);
	mgr.clearIndexForResource (index);
	return index;
}

void AlbumManager::indexStringForAlbum (int index, std::string str, int value) {
	auto &mgr = SearchManager::get();
	mgr.indexStringForResource (index, str, value);
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
	conn.release();

	int index = this->updateAlbumIndex (eventId);
	auto &searchMgr = SearchManager::get();
	searchMgr.setPictureForResource (index, picRes.resourceId);

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
	this->updateAlbumIndex (eventId);
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

int AlbumEventType::updateAlbumIndex (int albumId) {
	auto &albumMgr = AlbumManager::get();
	int index;
	auto albumData = this->getEvent (albumId);
	auto data = albumData.at ("data").get <AlbumEventType::Data>();
	auto parts = albumData.at ("participants").get <std::vector<ParticipantEntity>>();

	if (albumMgr.albumIsIndexed (albumId)) {
		index = albumMgr.clearAlbumIndex (
			albumId,
			data.author.name + " - " + data.album,
			data.description
		);
	} else {
		index = albumMgr.createAlbumSearchResIndex (
			albumId,
			data.author.name + " - " + data.album,
			data.description
		);
	}

	this->indexKeyword (index, data.album, SEARCH_VALUE_TITLE);
	this->indexKeyword (index, data.author.name, SEARCH_VALUE_AUTHOR);
	this->indexKeyword (index, data.description, SEARCH_VALUE_DESCRIPTION);

	for (const auto &i: parts) {
		this->indexKeyword (index, i.name, SEARCH_VALUE_PARTICIPANT);
	}

	return index;
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




UpdateAlbumResource::UpdateAlbumResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

ApiResponsePtr UpdateAlbumResource::processRequest (RequestData &rd, nlohmann::json body) {
	assertMethod (rd, "POST");
	auto &userMgr = UserManager::get();
	if (!userMgr.isLoggedIn (rd.getCookie (SESSION_ID))) return makeApiResponse (401);
	auto user = userMgr.getUserDataBySessionId (rd.getCookie (SESSION_ID));

	int albumId = getParameter <int> ("id", body);

	auto &albumMgr = AlbumManager::get();

	if (body.contains ("songs") && body.at ("songs").is_array()) {
		std::vector <int> currentSongIdsVector = albumMgr.getSongIdsForAlbum (albumId);
		std::set <int> toRemove (currentSongIdsVector.begin(), currentSongIdsVector.end());
		
		EventManager &eventMgr = EventManager::getManager();
		
		for (auto &i: body.at ("songs")) {
			i["type"] = "ep";
			if (i.contains ("id") == false || i.at ("id").is_null()) {
				// New song
				eventMgr.createEvent (i, user.id());
				// albumMgr.addSongToAlbumByReleaseId (albumId, release);

			} else {
				int songId = getParameter <int> ("id", i);
				// Old song with song_id = id;
				int eventId = SinglePublicationEventType::getEventIdForSong (songId);
				i ["id"] = eventId; // Id field must refer to the event id if we transfer the data to the events system
				eventMgr.updateEvent (i, user.id());
				toRemove.erase (songId);
			}
		}

		// Remove deleted songs
		for (auto i: toRemove) {
			int eventId = SinglePublicationEventType::getEventIdForSong (i);
			eventMgr.deleteEvent (eventId, user.id());
		}
	}

	return makeApiResponse (nlohmann::json{{"status", "success"}}, 200);
}

