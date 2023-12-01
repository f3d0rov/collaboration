#pragma once

#include <filesystem>

#include "../randomizer.hpp"
#include "../api_resource.hpp"

#define UPLOAD_ID_LEN 128
#define RESOURCE_PATH_LEN 64

class UploadedResourcesManager {
	public:
		struct GeneratedResource {
			std::string uploadId;
			int resourceId;
		};

	
	private:
		static std::unique_ptr<UploadedResourcesManager> _manager;
		UploadedResourcesManager ();

		std::filesystem::path _directory = "./";
		std::string _uploadUrl;
		std::string _downloadUrl;

		std::string generatePathForResource (int id, OwnedConnection &conn);
		bool isUploaded (int id, OwnedConnection &conn);
	public:
		UploadedResourcesManager (const UploadedResourcesManager &) = delete;
		UploadedResourcesManager (UploadedResourcesManager &&) = delete;

		static UploadedResourcesManager &get ();

		std::filesystem::path getPathForResource (int id);
		std::optional <std::filesystem::path> getPathIfUploaded (int id);
		std::optional <std::string> getFilenameIfUploaded (int id);
		void setUploadData (int id, int userId, std::string mime);
		int getIdByUploadId (std::string uploadId);
		void clearUploadLink (std::string uploadId);

		// returns an upload URL
		GeneratedResource createUploadableResource (bool genUploadLink = true);
		std::string generateUploadIdForResource (int id);
		void removeResource (int id);

		void setDirectory (std::string path);

		void setDownloadUri (std::string uri);
		std::string getDownloadUrl (std::string filename);
		std::string getDownloadUrl (int resourceId);

		void setUploadUrl (std::string uploadUrl);
		std::string getUploadUrl (std::string uploadId);
};



/***********
 * PUT @ /uploadpic?id=aFKJBSDFJJ124asdf...
 * data:image/png;base64,JASFLjfnkalff....
*/
class UploadResource: public Resource {
	public:
		UploadResource (mg_context *ctx, std::string uri);
		std::unique_ptr <_Response> processRequest (RequestData &rd) override;
};
