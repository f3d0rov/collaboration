
#include "user_auth.hpp"


std::string hashForPassword (std::string password, std::string salt) {
	std::string salted = password + salt;
	return sha3_256 (salted);
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
		if (iterations >= 10) {
			throw std::logic_error ("UserLoginResource::generateUniqueSessionId не может сгенерировать session_id");
		}
	} while (len != 0);

	return sid;
}

std::string UserLoginResource::authUserWithWork (int uid, std::string ip, pqxx::work &work) {
	std::string sessionId = UserLoginResource::generateUniqueSessionId (work);

	std::string removeOldCookiesOnDevice = std::string ("")
		+ "DELETE FROM user_login "
		+ "WHERE user_uid=" + std::to_string(uid)
		+ " AND device_ip=" + work.quote(ip) + ";";
	
	std::string addNewSession = std::string ("")
		+ "INSERT INTO user_login "
		+ "(session_id, user_uid, last_access, device_ip)"
		+ "VALUES (" 
			/* session_id */	+ work.quote (sessionId) + ","
			/* user_uid */ 		+ std::to_string (uid) + ","
			/* last_access */ 	+ "CURRENT_TIMESTAMP,"
			/* device_ip */ 	+ work.quote(ip)
		+ ");";
	
	work.exec (removeOldCookiesOnDevice);
	work.exec (addNewSession);
	return sessionId;
}

std::string UserLoginResource::authUser (int uid, std::string device_ip) {
	auto conn = database.connect ();
	pqxx::work work (*conn.conn);

	auto sessionId = UserLoginResource::authUserWithWork (uid, device_ip, work);

	work.commit();
	return sessionId;
}

ApiResponse UserLoginResource::successfulLogin (pqxx::work& work, int uid, std::string device_ip, std::string username) {
	std::string sessionId = UserLoginResource::authUserWithWork (uid, device_ip, work);
	return ApiResponse ({ {"username", username}, {"uid", uid} }).setCookie (
		"session_id", sessionId, true, 60*60*24*365
	).setCookie (
		"username", username, false, 60*60*24*365
	);
}

ApiResponse UserLoginResource::processRequest (RequestData &rd, nlohmann::json body) {
	if (rd.method != "POST") return ApiResponse (405);
	bool hasUsername = body.contains ("username"),
		hasPassword = body.contains ("password");
	if (!(hasUsername && hasPassword)) return ApiResponse (400); // Missing crucial data

	std::string username, password, device_id;
	try {
		username = lowercase(body["username"].template get<std::string>());
		password = lowercase(body["password"].template get<std::string>());
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
		ApiResponse result = this->successfulLogin (work, uid, rd.ip, username);
		work.commit();
		return result;
	}

	work.commit ();
	return ApiResponse ({{"status", "incorrect_password"}}, 200);
}


CheckUsernameAvailability::CheckUsernameAvailability (mg_context* ctx, std::string uri):
ApiResource (ctx, uri) {

}

bool CheckUsernameAvailability::isAvailable (std::string username) {
	auto conn = database.connect ();
	pqxx::work work (*conn.conn);
	std::string checkAvailabilityQuery = std::string ("SELECT uid FROM users WHERE username=") + work.quote (lowercase(username)) + ";";
	auto res = work.exec (checkAvailabilityQuery);
	bool free = res.size() == 0;

	work.commit();
	return free;
}

ApiResponse CheckUsernameAvailability::processRequest(RequestData &rd, nlohmann::json body) {
	if (rd.method != "POST") return ApiResponse (405);

	std::string username;
	if (!body.contains ("username")) return ApiResponse (400);
	try {
		username = body["username"].template get<std::string> ();
	} catch (nlohmann::json::exception& e) {
		return ApiResponse (400);
	}

	ApiResponse response (200);
	response.body["username"] = username;
	response.body["status"] = CheckUsernameAvailability::isAvailable (username) ? "free" : "taken";

	return response;
}

CheckEmailAvailability::CheckEmailAvailability (mg_context* ctx, std::string uri):
ApiResource (ctx, uri) {

}

bool CheckEmailAvailability::isAvailable (std::string email) {
	auto conn = database.connect ();
	pqxx::work work (*conn.conn);
	std::string checkAvailabilityQuery = std::string ("SELECT uid FROM users WHERE email=") + work.quote (lowercase(email)) + ";";
	auto res = work.exec (checkAvailabilityQuery);
	bool available = res.size() == 0;

	work.commit();
	return available;
}

ApiResponse CheckEmailAvailability::processRequest(RequestData &rd, nlohmann::json body) {
	if (rd.method != "POST") return ApiResponse (405);

	std::string email;
	if (!body.contains ("email")) return ApiResponse (400);
	try {
		email = body["email"].template get<std::string> ();
	} catch (nlohmann::json::exception& e) {
		return ApiResponse (400);
	}

	ApiResponse response (200);
	response.body["email"] = email;
	response.body["status"] = CheckEmailAvailability::isAvailable (email) ? "free" : "taken";

	return response;
}



UserRegisterResource::UserRegisterResource (mg_context* ctx, std::string uri):
ApiResource (ctx, uri) {

}

bool UserRegisterResource::checkPasswordFormat (std::string password) {
	return password.length() < 64;
}

bool UserRegisterResource::checkUsernameFormat (std::string username) {
	if (!(isAscii(username) && username.length() < 64)) return false;
	for (auto &i: username) {
		if (!std::isalnum (i)) return false; // If not a character or digit
	}
	return true;
}

bool UserRegisterResource::checkEmailFormat (std::string email) {
	// Fuck all
	int atSymbol = email.find ('@');
	int lastDot = email.find_last_of ('.');
	if (atSymbol == email.npos || lastDot == email.npos) return false;
	if (email.find ('@', atSymbol + 1) != email.npos) return false; // multiple '@'
	return lastDot > atSymbol;
}


std::string UserRegisterResource::generateSalt () {
	return randomizer.str (SALT_LEN);
}

ApiResponse UserRegisterResource::processRequest (RequestData &rd, nlohmann::json body){
	if (rd.method != "POST") return ApiResponse (405);
	if (!(body.contains ("username") && body.contains ("email") && body.contains ("password")))
		return ApiResponse (400);

	std::string username, email, password;

	try {
		username = lowercase(body["username"].template get <std::string>());
		email = lowercase(body["email"].template get <std::string>());
		password = lowercase(body["password"].template get <std::string>());
	} catch (nlohmann::json::exception& e) {
		return ApiResponse (400);
	}

	if (!this->checkEmailFormat (email)) return ApiResponse ({{"status", "incorrect_email_format"}}, 200);
	if (!this->checkUsernameFormat (username)) return ApiResponse ({{"status", "incorrect_username_format"}}, 200);
	if (!this->checkPasswordFormat (password)) return ApiResponse ({{"status", "incorrect_password_format"}}, 200);

	if (!CheckEmailAvailability::isAvailable (email))
		return ApiResponse ({ {"status", "email_taken"} }, 200);

	if (!CheckUsernameAvailability::isAvailable (username))
		return ApiResponse ({ {"status", "username_taken"} }, 200);

	std::string pass_salt = UserRegisterResource::generateSalt();
	std::string pass_hash = hashForPassword (password, pass_salt);

	auto conn = database.connect ();
	pqxx::work work (*conn.conn);

	std::string createUserQuery = std::string ("")
		+ "INSERT INTO users "
		+ "(username, email, pass_hash, pass_salt) "
		+ "VALUES ("
			/* username */	+ work.quote (username) + ","
			/* email */		+ work.quote (email) + ","
			/* pass_hash */ + work.quote (pass_hash) + ","
			/* pass_salt */ + work.quote (pass_salt)
		+ ");";
	work.exec (createUserQuery);

	std::string getUserIdQuery = std::string ("")
		+ "SELECT uid FROM users WHERE username=" + work.quote (username) + ";";
	auto user = work.exec1 (getUserIdQuery);
	int uid = user[0].as<int>();

	std::string sessionId = UserLoginResource::authUserWithWork (uid, rd.ip, work);
	work.commit();
	
	return ApiResponse ({ {"username", username}, {"uid", uid}, {"status", "success"} }).setCookie (
		"session_id", sessionId, true, 60*60*24*365 
	).setCookie (
		"username", username, false, 60*60*24*365
	);
}

CheckSessionResource::CheckSessionResource (mg_context* ctx, std::string uri):
ApiResource (ctx, uri) {

}

UsernameUid CheckSessionResource::checkSessionId (std::string sessionId) {
	auto conn = database.connect ();
	pqxx::work work (*conn.conn);

	UsernameUid uu;
	uu.valid = false;

	std::string checkSessionIdQuery = std::string ("")
		+ "SELECT user_uid "
		+ "FROM user_login "
		+ "WHERE session_id=" + work.quote (sessionId) + ";";

	pqxx::result res = work.exec (checkSessionIdQuery, "CheckSessionResource::checkSessionId::checkSessionIdQuery");

	if (res.size() == 0) return uu;
	uu.uid = res[0][0].as <int>();
	std::string getUsernameByUid = std::string () 
		+ "SELECT username, permission_level "
		+ "FROM users "
		+ "WHERE uid=" + std::to_string (uu.uid) + ";";
	
	pqxx::row unameRow = work.exec1 (getUsernameByUid, "CheckSessionResource::checkSessionId::getUsernameByUid");
	uu.username = unameRow[0].as <std::string>();
	uu.permissionLevel = unameRow[1].as <int>();
	uu.valid = true;
	return uu;
}


ApiResponse CheckSessionResource::processRequest (RequestData &rd, nlohmann::json body) {
	if (rd.method != "POST") return ApiResponse (405);
	if (!rd.setCookies.contains ("session_id")) return ApiResponse ({{"status", "unauthorized"}}, 200);
	UsernameUid uu = CheckSessionResource::checkSessionId (rd.setCookies["session_id"]);
	if (!uu.valid) return ApiResponse ({{"status", "unauthorized"}}, 200);
	return ApiResponse ({{"status", "authorized"}, {"username", uu.username}, {"uid", uu.uid}}, 200);
}
