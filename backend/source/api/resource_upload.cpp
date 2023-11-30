
#include "resource_upload.hpp"

std::unique_ptr <UploadedResourcesManager> UploadedResourcesManager::_manager;


UploadedResourcesManager::UploadedResourcesManager () {

}

UploadedResourcesManager &UploadedResourcesManager::get () {
	if (!(UploadedResourcesManager::_manager)) UploadedResourcesManager::_manager = std::unique_ptr <UploadedResourcesManager> (new UploadedResourcesManager{});
	return *UploadedResourcesManager::_manager;
}

std::string UploadedResourcesManager::generatePathForResource (int id, OwnedConnection &conn) {
	int count = 10;
	do {
		std::string rnd = randomizer.hex (RESOURCE_PATH_LEN);
		std::string query = "UPDATE uploaded_resources SET path="s + conn.quote (rnd) + " WHERE id=" + std::to_string (id) + ";";
		try {
			conn.exec (query);
			conn.commit();
			return rnd;
		} catch (pqxx::sql_error &e) {
			if (--count == 0) {
				throw std::logic_error ("UploadedResourcesManager::generatePathForResource: count == 0, last error: "s + e.what());
			}
			continue;
		}
	} while (1);
}

bool UploadedResourcesManager::isUploaded (int id, OwnedConnection &conn) {
	std::string checkQuery = "SELECT 1 FROM uploaded_resources WHERE id="s + std::to_string(id) + " AND path <> NULL;";
	auto result = conn.exec (checkQuery);
	return result.size() > 0;
}

std::filesystem::path UploadedResourcesManager::getPathForResource (int id) {
	auto conn = database.connect ();
	std::string checkQuery = "SELECT path FROM uploaded_resources WHERE id="s + std::to_string(id) + ";";
	auto result = conn.exec (checkQuery);

	std::string path = "";
	if (result.size() == 0 || result.at(0).at(0).is_null()) {
		path = this->generatePathForResource (id, conn);
	} else {
		path = result.at (0).at (0).as <std::string>();
	}
	return this->_directory / path;
}

std::optional <std::filesystem::path> UploadedResourcesManager::getPathIfUploaded (int id) {
	auto conn = database.connect ();
	std::string checkQuery = "SELECT path FROM uploaded_resources WHERE id="s + std::to_string(id) + ";";
	auto result = conn.exec (checkQuery);

	if (result.size() == 0 || result.at(0).at(0).is_null()) {
		return {};
	} else {
		return this->_directory / result.at (0).at (0).as <std::string>();
	}
}

void UploadedResourcesManager::setUploadData (int id, int userId, std::string mime) {
	auto conn = database.connect ();
	std::string checkQuery = "UPDATE uploaded_resources SET "
		"uploaded_by="s +  std::to_string (userId) + ","
		"uploaded_on=CURRENT_DATE,"
		"mime=" + conn.quoteDontEscapeHtml (mime)
		+ " WHERE id=" + std::to_string(id) + ";";
	conn.exec0 (checkQuery);
	conn.commit();
}

int UploadedResourcesManager::getIdByUploadId (std::string uploadId) {
	auto conn = database.connect ();
	std::string query = "SELECT resource_id FROM resource_upload_links WHERE valid_until>CURRENT_TIMESTAMP AND id="s + conn.quoteDontEscapeHtml (uploadId) + ";";
	auto row = conn.exec1 (query);
	return row.at(0).as <int>();
}

void UploadedResourcesManager::clearUploadLink (std::string uploadId) {
	auto conn = database.connect ();
	std::string query = "DELETE FROM resource_upload_links WHERE id="s + conn.quoteDontEscapeHtml (uploadId) + ";";
	if (conn.exec0 (query).affected_rows() != 1) throw std::logic_error ("UploadedResourcesManager::clearUploadLink: conn.exec0 (query).affected_rows() != 1");
}



UploadedResourcesManager::GeneratedResource UploadedResourcesManager::createUploadableResource (bool genUploadLink) {
	std::string query = "INSERT INTO uploaded_resources (path, uploaded_by, uploaded_on) VALUES (DEFAULT, DEFAULT, DEFAULT) RETURNING id;";
	auto conn = database.connect ();
	auto row = conn.exec1 (query);
	int id = row.at (0).as <int>();
	conn.commit();
	conn.release();

	UploadedResourcesManager::GeneratedResource gr;
	gr.resourceId = id;
	if (genUploadLink) gr.uploadId = this->generateUploadIdForResource (id);
	return gr;
}

std::string UploadedResourcesManager::generateUploadIdForResource (int id) {
	auto conn = database.connect();
	int count = 10;
	do {
		std::string rnd = randomizer.hex (UPLOAD_ID_LEN);
		std::string query = "INSERT INTO resource_upload_links (id, resource_id, valid_until) VALUES ("
			+ conn.quoteDontEscapeHtml (rnd) + ","
			+ std::to_string (id) + ","
			+ "CURRENT_TIMESTAMP + '1 minute'::INTERVAL);";
		try {
			conn.exec (query);
			conn.commit();
			return rnd;
		} catch (pqxx::sql_error &e) {
			if (--count == 0) {
				throw std::logic_error ("UploadedResourcesManager::generateUploadIdForResource: count == 0, last error: "s + e.what());
			}
			continue;
		}
	} while (1);
}

void UploadedResourcesManager::removeResource (int id) {
	auto conn = database.connect ();
	bool saved = this->isUploaded (id, conn);
	if (saved) {
		std::string checkQuery = "SELECT path FROM uploaded_resources WHERE id="s + std::to_string(id) + ";";
		auto row = conn.exec1 (checkQuery);
		std::filesystem::path path = this->_directory / row.at (0).as <std::string>();
		std::filesystem::remove (path);
	}
	std::string removeQuery = "DELETE FROM uploaded_resources WHERE id=" + std::to_string (id) + ";";
	conn.exec0 (removeQuery);
	conn.commit();
}


void UploadedResourcesManager::setDirectory (std::string path) {
	this->_directory = path;
}

void UploadedResourcesManager::setDownloadUri (std::string uri) {
	this->_downloadUrl = uri;
}

std::string UploadedResourcesManager::getDownloadUrl (std::string filename) {
	return this->_downloadUrl + "/" + filename;
}

void UploadedResourcesManager::setUploadUrl (std::string uploadUrl) {
	this->_uploadUrl = uploadUrl;
}

std::string UploadedResourcesManager::getUploadUrl (std::string uploadId) {
	return this->_uploadUrl + "?id=" + uploadId;
}





UploadResource::UploadResource (mg_context *ctx, std::string uri):
Resource (ctx, uri) {

}

std::unique_ptr <_Response> UploadResource::processRequest (RequestData &rd) {
	if (rd.method != "PUT") return std::make_unique <Response> (405);
	if (rd.query.contains ("id") == false) return std::make_unique <Response> (400);

	auto &userManager = UserManager::get();
	auto user = userManager.getUserDataBySessionId (rd.getCookie(SESSION_ID));
	if (!user.valid()) return makeApiResponse (nlohmann::json{}, 401);

	auto &resourceUploadManager = UploadedResourcesManager::get();
	std::string uploadId = rd.query ["id"];
	int resourceId = resourceUploadManager.getIdByUploadId (uploadId);
	std::filesystem::path resourcePath = resourceUploadManager.getPathForResource (resourceId);

	std::string headerEnd = ";base64,";
	int origin = rd.body.find (headerEnd) + headerEnd.length();
	int mimeStart = rd.body.find ("data:");
	std::string mime = rd.body.substr (mimeStart + 5, rd.body.find (";", mimeStart + 5) - mimeStart - 5);
	
	size_t decodedLen = rd.body.length() * 3 / 4;
	unsigned char *buffer = new unsigned char[decodedLen];
	if (-1 != mg_base64_decode (rd.body.c_str() + origin, rd.body.length() - origin, buffer, &decodedLen)) {
		logger << "UploadPictureResource::processRequest не смог декодировать данные в формате Base64" << std::endl;
		return std::make_unique <Response> (500);
	}

	std::ofstream of (resourcePath, std::ios::binary | std::ios::out);
	if (!of.is_open()) {
		logger << "Не смог открыть файл для записи: " << resourcePath << std::endl;
		return std::make_unique <Response> (500);
	}

	of.write ((const char *) buffer, decodedLen);
	of.close();
	delete[] buffer;

	resourceUploadManager.setUploadData (resourceId, user.id(), mime);
	resourceUploadManager.clearUploadLink (uploadId);

	logger << "Ресурс загружен в: " << resourcePath << "[" << decodedLen << "B]" << std::endl;
	return std::make_unique <Response> (200);
}
