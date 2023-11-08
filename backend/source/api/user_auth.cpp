
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

	std::string checkEmailConfirmed = std::string() 
		+ "SELECT uid FROM pending_email_confirmation WHERE uid=" + std::to_string (uid) + ";";
	auto result = work.exec (checkEmailConfirmed);
	if (result.size() != 0) return "";

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

std::unique_ptr<ApiResponse> UserLoginResource::successfulLogin (pqxx::work& work, int uid, std::string device_ip, std::string username) {
	std::string sessionId = UserLoginResource::authUserWithWork (uid, device_ip, work);
	auto res = std::make_unique<ApiResponse> (nlohmann::json{ {"username", username}, {"uid", uid}, {"status", "success"}}, 200);
	res->setCookie ("username", username, false, 60*60*24*365);
	res->setCookie ("session_id", sessionId, true, 60*60*24*365);
	return res;
}

std::unique_ptr<ApiResponse> UserLoginResource::processRequest (RequestData &rd, nlohmann::json body) {
	if (rd.method != "POST") return std::make_unique<ApiResponse> (405);
	bool hasUsername = body.contains ("username"),
		hasPassword = body.contains ("password");
	if (!(hasUsername && hasPassword)) return std::make_unique<ApiResponse> (400); // Missing crucial data

	std::string username, password, device_id;
	try {
		username = lowercase(body["username"].template get<std::string>());
		password = lowercase(body["password"].template get<std::string>());
	} catch (nlohmann::json::exception &e) {
		return std::make_unique<ApiResponse> (400);
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
		return std::make_unique<ApiResponse> (nlohmann::json{{"status", "no_such_user"}}, 200);
	} catch (std::exception &e) {
		logger << "Ошибка UserLoginResource::processRequest() при попытке получения данных пользователя: " << e.what() << std::endl;
		return std::make_unique<ApiResponse> (500);
	}

	std::string attemptHash = hashForPassword (password, pass_salt);
	if (attemptHash == pass_hash) {
		// Success!
		auto result = this->successfulLogin (work, uid, rd.ip, username);
		work.commit();
		return result;
	}

	work.commit ();
	return std::make_unique<ApiResponse>(nlohmann::json{{"status", "incorrect_password"}}, 200);
}



UserLogoutResource::UserLogoutResource (mg_context* ctx, std::string uri):
ApiResource (ctx, uri) {

}

std::unique_ptr<ApiResponse> UserLogoutResource::processRequest (RequestData& rd, nlohmann::json body) {
	if (rd.method != "POST") return std::make_unique<ApiResponse>(405);
	if (!rd.setCookies.contains ("session_id")) return std::make_unique<ApiResponse> (400);
	std::string session_id = rd.setCookies["session_id"];

	auto conn = database.connect();
	pqxx::work work (*conn.conn);

	std::string invalidateSessionQuery = std::string ()
		+ "DELETE FROM user_login "
		+ "WHERE session_id=" + work.quote (session_id) + ";";

	auto res = work.exec (invalidateSessionQuery);
	work.commit ();

	auto response = std::make_unique<ApiResponse>(200);
	response->setCookie ("session_id", "x", true, 0);
	return response;
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

std::unique_ptr<ApiResponse> CheckUsernameAvailability::processRequest(RequestData &rd, nlohmann::json body) {
	if (rd.method != "POST") return std::make_unique<ApiResponse> (405);

	std::string username;
	if (!body.contains ("username")) return std::make_unique<ApiResponse> (400);
	try {
		username = body["username"].template get<std::string> ();
	} catch (nlohmann::json::exception& e) {
		return std::make_unique<ApiResponse> (400);
	}

	auto response = std::make_unique<ApiResponse> (200);
	response->body["username"] = username;
	response->body["status"] = CheckUsernameAvailability::isAvailable (username) ? "free" : "taken";

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

std::unique_ptr<ApiResponse> CheckEmailAvailability::processRequest(RequestData &rd, nlohmann::json body) {
	if (rd.method != "POST") return std::make_unique<ApiResponse> (405);

	std::string email;
	if (!body.contains ("email")) return std::make_unique<ApiResponse> (400);
	try {
		email = body["email"].template get<std::string> ();
	} catch (nlohmann::json::exception& e) {
		return std::make_unique<ApiResponse> (400);
	}

	auto response = std::make_unique<ApiResponse> (200);
	response->body["email"] = email;
	response->body["status"] = CheckEmailAvailability::isAvailable (email) ? "free" : "taken";

	return response;
}



UserRegisterResource::UserRegisterResource (mg_context* ctx, std::string uri):
ApiResource (ctx, uri) {

}

bool UserRegisterResource::checkPasswordFormat (std::string password) {
	return password.length() >= 8 && password.length() < 64;
}

bool UserRegisterResource::checkUsernameFormat (std::string username) {
	if (!(isAscii(username) && username.length() < 64 && username.length() >= 3)) return false;
	for (auto &i: username) {
		if (!(std::isalnum (i) || std::string ("._-").find (i) != std::string::npos)) return false; // If not a character or digit
	}
	return true;
}

bool UserRegisterResource::checkEmailFormat (std::string email) {
	// Fuck all
	int atSymbol = email.find ('@');
	int lastDot = email.find_last_of ('.');
	if (atSymbol == email.npos || lastDot == email.npos) return false;
	if (email.find ('@', atSymbol + 1) != email.npos) return false; // multiple '@'
	try {
		jed_utils::MessageAddress addr (email.c_str());
	} catch (...) {
		return false;
	}
	return lastDot > atSymbol;
}


std::string UserRegisterResource::generateSalt () {
	return randomizer.str (SALT_LEN);
}

std::unique_ptr<ApiResponse> UserRegisterResource::processRequest (RequestData &rd, nlohmann::json body){
	if (rd.method != "POST") return std::make_unique<ApiResponse> (405);
	if (!(body.contains ("username") && body.contains ("email") && body.contains ("password")))
		return std::make_unique<ApiResponse> (400);

	std::string username, email, password;

	try {
		username = lowercase(body["username"].template get <std::string>());
		email = lowercase(body["email"].template get <std::string>());
		password = lowercase(body["password"].template get <std::string>());
	} catch (nlohmann::json::exception& e) {
		return std::make_unique<ApiResponse> (400);
	}

	if (!this->checkEmailFormat (email)) return std::make_unique<ApiResponse> (nlohmann::json{{"status", "incorrect_email_format"}}, 200);
	if (!this->checkUsernameFormat (username)) return std::make_unique<ApiResponse> (nlohmann::json{{"status", "incorrect_username_format"}}, 200);
	if (!this->checkPasswordFormat (password)) return std::make_unique<ApiResponse> (nlohmann::json{{"status", "incorrect_password_format"}}, 200);

	if (!CheckEmailAvailability::isAvailable (email))
		return std::make_unique<ApiResponse> (nlohmann::json{ {"status", "email_taken"} }, 200);

	if (!CheckUsernameAvailability::isAvailable (username))
		return std::make_unique<ApiResponse> (nlohmann::json{ {"status", "username_taken"} }, 200);

	std::string pass_salt = UserRegisterResource::generateSalt();
	std::string pass_hash = hashForPassword (password, pass_salt);

	auto conn = database.connect ();
	pqxx::work work (*conn.conn);

	std::string createUserQuery = std::string ("")
		+ "INSERT INTO users "
		+ "(username, email, pass_hash, pass_salt) "
		+ "VALUES ("
			/* username */	+ work.quote (escapeHTML(username)) + ","
			/* email */		+ work.quote (email) + ","
			/* pass_hash */ + work.quote (pass_hash) + ","
			/* pass_salt */ + work.quote (pass_salt)
		+ ");";
	
	work.exec (createUserQuery);

	std::string getUserIdQuery = std::string ("")
		+ "SELECT uid FROM users WHERE username=" + work.quote (username) + ";";
	auto user = work.exec1 (getUserIdQuery);
	int uid = user[0].as<int>();

	std::string confirmationId = randomizer.hex (128);
	std::string awaitEmailConfirmationQuery = std::string()
		+ "INSERT INTO pending_email_confirmation "
		+ "(uid, confirmation_id, valid_until) "
		+ "VALUES ("
		+ std::to_string (uid) + ","
		+ work.quote (confirmationId) + ","
		+ "CURRENT_TIMESTAMP + interval '2 hours');";
	work.exec (awaitEmailConfirmationQuery);
	work.commit();

	mailer.sendHtmlLetter (
		email,
		"Добро пожаловать в COLLABORATION.",
		"confirm_email.html",
		{
			{ "{USERNAME}", username },
			{ "{REGISTER_URL}", common.domain + "/confirm?id=" + confirmationId }
		}
	);
	
	return std::make_unique<ApiResponse> (nlohmann::json{ {"status", "success"} }, 200);
}



ConfirmRegistrationResource::ConfirmRegistrationResource (mg_context* ctx, std::string uri):
Resource (ctx, uri) {
	logger << "Интерфейс API: \"" + uri + "\"" << std::endl;
}

std::unique_ptr<_Response> ConfirmRegistrationResource::processRequest (RequestData &rd) {
	auto response = std::make_unique <Response> (500);
	response->redirect(common.http + "://" + common.domain + "/");
	
	if (!rd.query.contains ("id")) return response;

	std::string id = rd.query["id"];

	auto conn = database.connect();
	pqxx::work work (*conn.conn);

	std::string checkIdQuery = std::string()
		+ "SELECT uid "
		+ "FROM pending_email_confirmation "
		+ "WHERE confirmation_id=" + work.quote (id)
		+ " AND valid_until > CURRENT_TIMESTAMP;";
	auto result = work.exec (checkIdQuery);
	if (result.size() == 0) return response;

	int uid = result[0].at(0).as <int>();
	std::string removeId = std::string() 
		+ "DELETE FROM pending_email_confirmation "
		+ "WHERE confirmation_id=" + work.quote (id);
	work.exec (removeId);

	std::string sessionId = UserLoginResource::authUserWithWork (uid, rd.ip, work);
	
	std::string getUsernameByUid = std::string () 
		+ "SELECT username "
		+ "FROM users "
		+ "WHERE uid=" + std::to_string (uid) + ";";
	auto row = work.exec1 (getUsernameByUid);
	std::string username = row.at(0).as <std::string> ();
	
	response->setCookie ("session_id", sessionId, true);
	response->setCookie ("username", username, false);

	work.commit();
	return response;
}



CheckSessionResource::CheckSessionResource (mg_context* ctx, std::string uri):
ApiResource (ctx, uri) {

}

UsernameUid CheckSessionResource::checkSessionId (std::string sessionId) {
	if (sessionId == "") {
		UsernameUid uuf;
		uuf.valid = false;
		return uuf;
	}

	auto conn = database.connect ();
	pqxx::work work (*conn.conn);

	UsernameUid uu;
	uu.valid = false;

	std::string checkSessionIdQuery = std::string ("")
		+ "SELECT user_uid "
		+ "FROM user_login "
		+ "WHERE session_id=" + work.quote (sessionId) + ";";

	pqxx::result res = work.exec (checkSessionIdQuery);

	if (res.size() == 0) return uu;
	uu.uid = res[0][0].as <int>();
	std::string getUsernameByUid = std::string () 
		+ "SELECT username, permission_level "
		+ "FROM users "
		+ "WHERE uid=" + std::to_string (uu.uid) + ";";
	
	pqxx::row unameRow = work.exec1 (getUsernameByUid);
	uu.username = unameRow[0].as <std::string>();
	uu.permissionLevel = unameRow[1].as <int>();
	uu.valid = true;

	std::string setLastAccessTimestampUser = std::string()
		+ "UPDATE users "
		+ "SET last_access=CURRENT_TIMESTAMP "
		+ "WHERE uid=" + std::to_string (uu.uid) + ";";
	
	std::string setLastAccessTimestampSession = std::string()
		+ "UPDATE user_login "
		+ "SET last_access=CURRENT_TIMESTAMP "
		+ "WHERE session_id=" + work.quote (sessionId) + ";";

	work.exec (setLastAccessTimestampUser);
	work.exec (setLastAccessTimestampSession);
	work.commit ();
	return uu;
}	

UsernameUid CheckSessionResource::checkSessionId (RequestData &rd) {
	UsernameUid uu;
	if (!rd.setCookies.contains ("session_id")) {
		uu.valid = false;
		return uu;
	} else {
		return CheckSessionResource::checkSessionId (rd.setCookies["session_id"]);
	}
}

bool CheckSessionResource::isLoggedIn (RequestData &rd, int minPermissionLevel) {
	if (!rd.setCookies.contains ("session_id")) return false;
	std::string sessionId = rd.setCookies ["session_id"];
	UsernameUid user = CheckSessionResource::checkSessionId (sessionId);
	return user.valid && (user.permissionLevel >= minPermissionLevel);
}



std::unique_ptr<ApiResponse> CheckSessionResource::processRequest (RequestData &rd, nlohmann::json body) {
	if (rd.method != "POST") return std::make_unique<ApiResponse> (405);
	if (!rd.setCookies.contains ("session_id")) {
		auto response = std::make_unique<ApiResponse> (nlohmann::json{{"status", "unauthorized"}}, 200);
		response->setCookie ("session_id", "", true, 0);
		return response;
	}

	UsernameUid uu = CheckSessionResource::checkSessionId (rd.setCookies["session_id"]);
	if (!uu.valid){
		auto response = std::make_unique<ApiResponse> (nlohmann::json{{"status", "unauthorized"}}, 200);
		response->setCookie ("session_id", "", true, 0);
		return response;
	}
	return std::make_unique<ApiResponse> (nlohmann::json{{"status", "authorized"}, {"username", uu.username}, {"uid", uu.uid}}, 200);
}
