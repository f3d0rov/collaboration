#pragma once

#include "../api_resource.hpp"
#include "events.hpp"
#include "event_types.hpp"

class AlbumManager;
class AlbumEventType;
class AlbumDataResource;


class AlbumManager {
	private:
		AlbumManager ();
		static std::unique_ptr <AlbumManager> _manager;

	public:

		AlbumManager (const AlbumManager &mgr) = delete;
		AlbumManager (AlbumManager &&mgr) = delete;

		static AlbumManager &get ();

		void addSongToAlbumByReleaseId (int albumId, int release); // returns song id
		// void removeSongFromAlbum (int albumId, int songId);
		// void setSongOrder (int songId, int orderIndex);
		// void updateSongForAlbum (int albumId, nlohmann::json songData);

		std::vector <nlohmann::json> getSongsForAlbum (int albumId);
		std::vector <int> getSongIdsForAlbum (int albumId);
		nlohmann::json getAlbumData (int albumId);

		int getAlbumPictureResourceId (int id);
		std::string getAlbumPictureUrl (int albumId);
};


class AlbumEventType: public AllEntitiesEventType {
	private:
		struct Data {
			ParticipantEntity author;
			std::string date;
			std::string album;
			std::string description;
		};

		friend void from_json (const nlohmann::json &j, AlbumEventType::Data &d);
		friend void to_json (nlohmann::json &j, const AlbumEventType::Data &d);
	public: 
		std::string getTypeName () const override;
		std::string getDisplayName () const override;
		std::string getTitleFormat () const override;

		std::vector <InputTypeDescriptor> getInputs () const override;
		
		int createEvent (nlohmann::json &data) override;
		nlohmann::json getEvent (int id) override;
		int updateEvent (nlohmann::json &data) override;

		int getAuthorForEvent (OwnedConnection &work, int eventId);
};

void from_json (const nlohmann::json &j, AlbumEventType::Data &d);
void to_json (nlohmann::json &j, const AlbumEventType::Data &d);


class AlbumDataResource: public ApiResource {
	public:
		AlbumDataResource (mg_context *ctx, std::string uri);
		ApiResponsePtr processRequest (RequestData &rd, nlohmann::json body) override;
};


/******************
 *  POST {id: <album id>}
 * -> {"url": <upload url>}
******************/
class RequestAlbumImageChangeResource: public ApiResource {
	public:
		RequestAlbumImageChangeResource (mg_context *ctx, std::string uri);
		ApiResponsePtr processRequest (RequestData &rd, nlohmann::json body) override;
};


class UpdateAlbumResource: public ApiResource {
	public:
		UpdateAlbumResource (mg_context *ctx, std::string uri);
		ApiResponsePtr processRequest (RequestData &rd, nlohmann::json body) override;
};
