
#include "user_auth.hpp"


std::string hashForPassword (std::string password, std::string salt) {
	std::string salted = password + salt;
	return sha256 (salted);
}


UserLoginResource::UserLoginResource (mg_context* ctx, std::string uri):
ApiResource (ctx, uri) {

}

std::string UserLoginResource::generateUniqueSessionId (pqxx::work& work) {
	int len = 10;
	std::string sid;

	int iterations = 0;

	do {
		sid = randomizer.hex (SESSION_ID_LEN);
		std::string query = std::string ("SELECT session_id FROM user_login where session_id=") + work.quote (sid) + ";";
		auto rows = work.exec (query);
		len = rows.size ();
		
		iterations++;
		if (iterations >= 100) {
			throw std::logic_error ("UserLoginResource::generateUniqueSessionId не может сгенерировать session_id");
		}
	} while (len != 0);

	return sid;
}

ApiResponse UserLoginResource::successfulLogin (pqxx::work& work, int uid, std::string device_id, std::string username) {
	std::string sessionId = this->generateUniqueSessionId (work);

	std::string removeOldCookiesOnDevice = std::string ("")
		+ "DELETE FROM user_login "
		+ "WHERE user_uid=" + std::to_string(uid)
		+ "&&device_id=" + device_id + ";";
	
	std::string addNewSession = std::string ("")
		+ "INSERT INTO user_login "
		+ "(session_id, user_uid, last_access, device_id)"
		+ "VALUES (" 
			/* session_id */	+ work.quote (sessionId) + ","
			/* user_uid */ 		+ std::to_string (uid) + ","
			/* last_access */ 	+ "CURRENT_TIMESTAMP,"
			/* device_id */ 	+ device_id
		+ ");";
	
	work.exec (removeOldCookiesOnDevice);
	work.exec (addNewSession);

	return ApiResponse ({ {"session_id", sessionId}, {"username", username}, {"uid", uid} });
}

ApiResponse UserLoginResource::processRequest (std::string method, std::string uri, nlohmann::json body) {
	if (method != "POST") return ApiResponse (405);
	bool hasUsername = body.contains ("username"),
		hasPassword = body.contains ("password"),
		hasDeviceId = body.contains ("device_id");
	if (!(hasUsername && hasPassword && hasDeviceId)) return ApiResponse (400); // Missing crucial data

	std::string username, password, device_id;
	try {
		username = body["username"].template get<std::string>();
		password = body["password"].template get<std::string>();
		device_id = body["device_id"].template get<std::string>();
	} catch (nlohmann::json::exception &e) {
		return ApiResponse (400);
	}

	auto conn = database.connect();
	auto work = pqxx::work (*conn.conn);

	std::string qUsername = work.quote (username);
	// std::string qDevice_id = work.quote (device_id);

	std::string getUserdataQuery = std::string ("")
		+ "SELECT pass_hash, pass_salt, uid "
		+ "FROM users "
		+ "WHERE username=" + qUsername + ";";

	std::string pass_hash, pass_salt;
	int uid;

	try {
		auto row = work.exec1 (getUserdataQuery);
		pass_hash = row[0].as <std::string>();
		pass_salt = row[1].as <std::string>();
		uid = row[2].as <int>();
	} catch (pqxx::unexpected_rows& e) {
		// pqxx::unexpected_rows не передает количество, предполагаем 0 - пользователь с указанным `username` не существует
		return ApiResponse ({{"status", "no_such_user"}}, 200);
	} catch (std::exception &e) {
		logger << "Ошибка UserLoginResource::processRequest() при попытке получения данных пользователя: " << e.what() << std::endl;
		return ApiResponse (500);
	}

	std::string attemptHash = hashForPassword (password, pass_salt);
	if (attemptHash == pass_hash) {
		// Success!
		ApiResponse result = this->successfulLogin (work, uid, device_id, username);
		work.commit();
		return result;
	}

	work.commit ();
	return ApiResponse ({{"status", "incorrect_password"}}, 200);
}


CheckUsernameAvailability::CheckUsernameAvailability (mg_context* ctx, std::string uri):
ApiResource (ctx, uri) {

}

ApiResponse CheckUsernameAvailability::processRequest(std::string method, std::string uri, nlohmann::json body) {
	if (method != "POST") return ApiResponse (405);

	std::string username;
	if (!body.contains ("username")) return ApiResponse (400);
	try {
		username = body["username"].template get<std::string> ();
	} catch (nlohmann::json::exception& e) {
		return ApiResponse (400);
	}

	auto conn = database.connect ();
	pqxx::work work (*conn.conn);
	std::string checkAvailabilityQuery = std::string ("SELECT uid FROM users WHERE username=") + work.quote (username) + ";";
	auto res = work.exec (checkAvailabilityQuery);
	bool free = res.size() == 0;

	ApiResponse response (200);
	response.body["username"] = username;
	response.body["status"] = free ? "free" : "taken";

	work.commit();
	return response;
}

CheckEmailAvailability::CheckEmailAvailability (mg_context* ctx, std::string uri):
ApiResource (ctx, uri) {

}

ApiResponse CheckEmailAvailability::processRequest(std::string method, std::string uri, nlohmann::json body) {
	if (method != "POST") return ApiResponse (405);

	std::string email;
	if (!body.contains ("email")) return ApiResponse (400);
	try {
		email = body["email"].template get<std::string> ();
	} catch (nlohmann::json::exception& e) {
		return ApiResponse (400);
	}

	auto conn = database.connect ();
	pqxx::work work (*conn.conn);
	std::string checkAvailabilityQuery = std::string ("SELECT uid FROM users WHERE email=") + work.quote (email) + ";";
	auto res = work.exec (checkAvailabilityQuery);
	bool free = res.size() == 0;

	ApiResponse response (200);
	response.body["email"] = email;
	response.body["status"] = free ? "free" : "taken";

	work.commit();
	return response;
}