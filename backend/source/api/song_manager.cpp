
#include "song_manager.hpp"


SongData::SongData (const SongData::SongDatabaseData &sdd) {
	this->songId = sdd.songId;
	if (!sdd.releaseEventId.isNull()) this->eventId = sdd.releaseEventId;
	this->name = sdd.name;
	this->description = sdd.description;

	this->author.entityId = sdd.authorId;
	this->author.name = sdd.authorName;
	this->author.created = !sdd.authorAwaitsCreation;

	this->releasedOn = sdd.releaseDate;
	
	if (!sdd.albumId.isNull()) {
		this->album->id = sdd.albumId;
		this->album->index = sdd.albumIndex;
	}	
}


/* static */ SongData SongData::fromSongId (int id) {
	auto conn = database.connect();
	SongData::SongDatabaseData dbdata;
	GetQuery query;

	query.select ({
		dbdata.songId, dbdata.name, 
		dbdata.authorId, dbdata.authorName, dbdata.authorAwaitsCreation,
		dbdata.albumId, dbdata.albumIndex, 
		dbdata.releaseEventId, dbdata.description,
		dbdata.releaseDate
	})
		.from (SONGS_TABLE)
		.join ("entities", Condition::equals (dbdata.authorId, dbdata.entityId))
		.loJoin ("events", Condition::equals (dbdata.releaseEventId, dbdata.eventId))
		.where (Condition::equals(dbdata.songId, id))
		.exec (conn);
	
	if (query.size() == 0) throw std::runtime_error ("Нет песни с заданным id");
	return SongData {dbdata};
}

/* static */ SongData SongData::fromEventId (int id) {
	auto conn = database.connect();
	SongData::SongDatabaseData dbdata;
	GetQuery query;

	query.select ({dbdata.songId, dbdata.name, dbdata.authorId, dbdata.albumId, dbdata.albumIndex, dbdata.releaseEventId, dbdata.releaseDate})
		.from (SONGS_TABLE)
		.where (Condition::equals(dbdata.releaseEventId, id))
		.exec (conn);
	
	if (query.size() == 0) throw std::runtime_error ("Нет песни с заданным id");
	return SongData {dbdata};
}

void toJson (nlohmann::json &json, const SongData &songData) {
	json ["id"] = songData.songId;
	json ["song"] = songData.name;
	json ["author"] = songData.author;
	json ["description"] = songData.description;
}

