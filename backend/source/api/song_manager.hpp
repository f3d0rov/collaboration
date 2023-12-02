#pragma once

#include "../database.hpp"
#include "../query_builder.hpp"
#include "events.hpp"

#define SONGS_TABLE "songs"

class SongData {
	private:
		class SongDatabaseData {
			public:
				QueryColumn <int> songId {SONGS_TABLE, "id"};
				QueryColumn <std::string> name {SONGS_TABLE, "title"};

				QueryColumn <int> authorId {SONGS_TABLE, "author"};
				QueryColumn <int> entityId {"entities", "id"}; 
				QueryColumn <std::string> authorName {"entities", "name"};
				QueryColumn <bool> authorAwaitsCreation {"entities", "awaits_creation"};

				QueryColumn <int> albumId {SONGS_TABLE, "album"};
				QueryColumn <int> albumIndex {SONGS_TABLE, "album_index"};

				QueryColumn <std::string> releaseDate {SONGS_TABLE, "release_date"};

				QueryColumn <int> releaseEventId {SONGS_TABLE, "release"};
				QueryColumn <std::string> description {"events", "description"};
				QueryColumn <int> eventId {"events", "id"};
		};
	
		SongData (const SongDatabaseData &sdd);

	public:
		struct AlbumData {
			int id;
			int index;
		};
		
		static SongData fromSongId (int id);
		static SongData fromEventId (int id);

		int songId;
		std::optional <int> eventId;
		std::string name, description;

		ParticipantEntity author;
		std::string releasedOn;
		
		std::optional <SongData::AlbumData> album;
};

void toJson (nlohmann::json &json, const SongData &songData);