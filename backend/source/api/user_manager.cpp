
#include "user_manager.hpp"


UserData::UserData (bool):
_valid (false) {

}

UserData::UserData (int uid, OwnedConnection &conn) {
	if (uid < 0) {
		this->_valid = false;
		return;
	}

	std::string getUserDataQuery = 
		"SELECT username, email, last_access, permission_level FROM users WHERE uid="s + std::to_string (uid) + ";";
	auto result = conn.exec (getUserDataQuery);

	if (result.size() == 1) {
		auto row = result.at (0);
		this->_username = row.at ("username").as <std::string>();
		this->_email = row.at ("email").as <std::string>();
		this->_lastActionDate = row.at ("last_access").as <std::string>();
		this->_accessLevel = row.at ("permission_level").as <int>();
		this->_id = uid;
		this->_valid = true;
	} else if (result.size() == 0) {
		this->_valid = false;
	} else {
		throw std::logic_error ("UserData::UserData: result.size() > 1");
	}
}

UserData::UserData (UserData &&ud):
_username (ud._username),
_email (ud._email),
_lastActionDate (ud._lastActionDate),
_accessLevel (ud._accessLevel),
_id (ud._id),
_valid (ud._valid) {

}

UserData::UserData (const UserData &ud):
_username (ud._username),
_email (ud._email),
_lastActionDate (ud._lastActionDate),
_accessLevel (ud._accessLevel),
_id (ud._id),
_valid (ud._valid) {

}

int UserData::id () const {
	return this->_id;
}

std::string UserData::username () const {
	return this->_username;
}

std::string UserData::email () const {
	return this->_email;
}

int UserData::accessLevel () const {
	return this->_accessLevel;
}

std::string UserData::lastActionDate () const {
	return this->_lastActionDate;
}

bool UserData::hasAccessLevel (int level) const {
	return this->_valid && (this->_accessLevel <= level);
}

bool UserData::valid () const {
	return this->_valid;
}

UserData::operator int() const {
	return this->_valid ? this->_id : -1;
}




std::unique_ptr <UserManager> UserManager::_manager;

UserManager::UserManager () {

}

UserManager &UserManager::get () {
	if (!UserManager::_manager) UserManager::_manager = std::unique_ptr <UserManager>(new UserManager{});
	return *UserManager::_manager;
}


std::string UserManager::generateSalt () {
	return randomizer.str (SALT_LEN);
}
		
std::string UserManager::hashForPassword (std::string password, std::string salt) {
	std::string salted = password + salt;
	return sha3_256 (salted);
}

bool UserManager::checkPassword (int uid, std::string password, OwnedConnection &conn) { 
	std::string query = "SELECT pass_hash, pass_salt FROM users WHERE uid=" + std::to_string (uid) + ";";
	auto res = conn.exec (query);
	if (res.size() != 1) return false;
	std::string hash, salt;
	hash = res.at (0).at ("pass_hash").as <std::string>();
	salt = res.at (0).at ("pass_salt").as <std::string>();
	return this->hashForPassword (password, salt) == hash;
}

bool UserManager::checkPasswordFormat (std::string password) {
	return password.length() >= 8 && password.length() < 64;
}

bool UserManager::checkUsernameFormat (std::string username) {
	if (!(isAscii(username) && username.length() < 64 && username.length() >= 3)) return false;
	for (auto &i: username) {
		if (!(std::isalnum (i) || std::string ("._-").find (i) != std::string::npos)) return false; // If not a character or digit or one of ._-
	}
	return true;
}

bool UserManager::checkEmailFormat (std::string email) {
	// bool jed_utils::MessageAddress::isEmailAddressValid is private and non-static, so we're using the second best thing
	try {
		jed_utils::MessageAddress addr (email.c_str());
		// Check happens here:
		// https://github.com/jeremydumais/CPP-SMTPClient-library/blob/master/src/messageaddress.cpp, line #18
		return true;
	} catch (...) {
		return false;
	}
}

bool UserManager::emailConfirmed (int userId, OwnedConnection &conn) {
	std::string checkEmailConfirmed = 
		"SELECT uid FROM pending_email_confirmation WHERE uid="s + std::to_string (userId) + ";";
	auto result = conn.exec (checkEmailConfirmed);
	if (result.size() != 0) return false;
	return true;
}

std::string UserManager::generateSessionId (OwnedConnection& conn) {
	int len = 10;
	std::string sid;

	int iterations = 0;

	do {
		// Not really checking that 128 random bytes are unique in the table
		// Just that randomizer works properly
		sid = randomizer.hex (SESSION_ID_LEN);
		std::string query = std::string ("SELECT session_id FROM user_login where session_id=") + conn.quoteDontEscapeHtml (sid) + ";";
		auto rows = conn.exec (query);
		len = rows.size ();
		
		iterations++;
		if (iterations >= 10) {
			throw std::logic_error ("UserLoginResource::generateUniqueSessionId не может сгенерировать session_id");
		}
	} while (len != 0);

	return sid;
}

int UserManager::getUserIdBySessionId (std::string sessionId, OwnedConnection &conn) {
	std::string checkSessionIdQuery =
		"SELECT user_uid "
		"FROM user_login "
		"WHERE session_id="s + conn.quoteDontEscapeHtml (sessionId) + ";";

	pqxx::result res = conn.exec (checkSessionIdQuery);
	if (res.size() == 0) return -1;
	if (res.size() > 1) throw std::logic_error ("UserManager::getUserDataBySessionId: res.size() > 1");

	return res.at (0).at (0).as <int>();
}

int UserManager::getUserIdByUsername (std::string username, OwnedConnection &conn) {
	std::string query = "SELECT uid FROM users WHERE username ILIKE " + conn.quote (username) + ";";
	auto result = conn.exec (query);
	if (result.size() != 1) throw UserMistakeException ("Не существует пользователя с заданным именем");
	return result.at (0).at (0).as <int>();
}

std::string UserManager::authUserWithWork (int uid, std::string device_ip, OwnedConnection &conn) {
	std::string sessionId = this->generateSessionId (conn);
	if (!this->emailConfirmed (uid, conn)) throw UserMistakeException ("Невозможно авторизироваться - адрес почты не подтвержден");

	std::string addNewSession =
		"INSERT INTO user_login "
		"(session_id, user_uid, last_access, device_ip)"
		"VALUES ("s
			/* session_id */	+ conn.quoteDontEscapeHtml (sessionId) + ","
			/* user_uid */ 		+ std::to_string (uid) + ","
			/* last_access */ 	+ "CURRENT_TIMESTAMP,"
			/* device_ip */ 	+ conn.quote (device_ip)
		+ ");";
	
	conn.exec (addNewSession);
	this->updateUserLastActionTimeWithWork (uid, conn);

	#if 0 // TODO: Could just be second device from the same network. Need a better method.
		std::string removeOldCookiesOnDevice = std::string ("")
			+ "DELETE FROM user_login "
			+ "WHERE user_uid=" + std::to_string(uid)
			+ " AND device_ip=" + conn.quote (device_ip) + ";";
		conn.exec (removeOldCookiesOnDevice);
	#endif

	return sessionId;
}


UserData UserManager::getUserDataWithWork (int uid, OwnedConnection &conn) {
	return UserData (uid, conn);
}

void UserManager::updateUserLastActionTimeWithWork (int uid, OwnedConnection &conn) {
	std::string query = "UPDATE users SET last_access=CURRENT_TIMESTAMP WHERE uid="s + std::to_string(uid) + ";";
	conn.exec (query);
}

bool UserManager::emailIsAvailableWithConn (std::string email, OwnedConnection &conn) {
	std::string checkAvailabilityQuery = std::string ("SELECT uid FROM users WHERE email=") + conn.quote (lowercase(email)) + ";";
	auto res = conn.exec (checkAvailabilityQuery);
	return res.size() == 0;
}

bool UserManager::usernameIsAvailableWithConn (std::string username, OwnedConnection &conn) {
	std::string checkAvailabilityQuery = std::string ("SELECT uid FROM users WHERE username ILIKE ") + conn.quote (username) + ";";
	auto res = conn.exec (checkAvailabilityQuery);
	return res.size() == 0;
	// conn.commit(); // ! Don't commit without edits, just in case we messed up string escapes
}

void UserManager::sendConfirmationEmail (std::string username, std::string email, std::string confirmId) {
	mailer.sendHtmlLetter (
		email,
		"Добро пожаловать в COLLABORATION.",
		"confirm_email.html",
		{
			{ "{USERNAME}", username },
			{ "{REGISTER_URL}", common.domain + "/confirm?id=" + confirmId }
		}
	);	
}

std::string UserManager::authUser (int userId, std::string deviceIp) {
	auto conn = database.connect ();
	auto sessionId = this->authUserWithWork (userId, deviceIp, conn);
	conn.commit();
	return sessionId;
}

std::string UserManager::loginUser (std::string username, std::string password, std::string deviceIp) {
	auto conn = database.connect ();
	int uid = this->getUserIdByUsername (username, conn);
	if (this->checkPassword (uid, password, conn)) {
		return this->authUser (uid, deviceIp);
	} else {
		throw UserMistakeException ("Неверный пароль");
	}
}

void UserManager::logoutUser (std::string sessionId) {
	if (sessionId == "") return;
	auto conn = database.connect();
	std::string invalidateSessionQuery =
		"DELETE FROM user_login "
		"WHERE session_id="s + conn.quoteDontEscapeHtml (sessionId) + ";";
	auto res = conn.exec (invalidateSessionQuery);
	conn.commit ();
}


bool UserManager::emailIsAvailable (std::string email) {
	auto conn = database.connect ();
	return this->emailIsAvailableWithConn (email, conn);
}

bool UserManager::usernameIsAvailable (std::string username) {
	auto conn = database.connect ();
	return this->usernameIsAvailableWithConn (username, conn);
}

int UserManager::registerUser (std::string username, std::string password, std::string email) {
	if (!this->checkEmailFormat (email)) throw UserMistakeException ("Неверный формат письма");
	if (!this->checkUsernameFormat (username)) throw UserMistakeException ("Неверный формат имени пользователя");
	if (!this->checkPasswordFormat (password)) throw UserMistakeException ("Неверный формат пароля");

	auto conn = database.connect ();

	if (!this->emailIsAvailableWithConn (email, conn))
		throw UserMistakeException ("Адрес почты занят");

	if (!this->usernameIsAvailableWithConn (username, conn))
		throw UserMistakeException ("Имя пользователя занято");

	std::string pass_salt = this->generateSalt();
	std::string pass_hash = this->hashForPassword (password, pass_salt);


	std::string createUserQuery =
		"INSERT INTO users "
		"(username, email, pass_hash, pass_salt) "
		"VALUES ("s
			/* username */	+ conn.quote (username) + ","
			/* email */		+ conn.quote (email) + ","
			/* pass_hash */ + conn.quoteDontEscapeHtml (pass_hash) + ","
			/* pass_salt */ + conn.quoteDontEscapeHtml (pass_salt)
		+ ") RETURNING uid;";
	
	pqxx::row user = conn.exec1 (createUserQuery);
	int uid = user.at (0).as <int>();

	std::string confirmationId = randomizer.hex (128);
	std::string awaitEmailConfirmationQuery =
		"INSERT INTO pending_email_confirmation "
		"(uid, confirmation_id, valid_until) "
		"VALUES ("s
		+ /* uid */ 		std::to_string (uid) + ","
		+ /* confirm_id */ 	conn.quoteDontEscapeHtml (confirmationId) + ","
		+ /* valid_until */ "CURRENT_TIMESTAMP + interval '2 hours');";
	conn.exec (awaitEmailConfirmationQuery);
	conn.commit();
	conn.release();

	this->sendConfirmationEmail (username, email, confirmationId);
	return uid;
}

int UserManager::confirmUserEmail (std::string confirmId) {
	auto conn = database.connect();
	std::string checkIdQuery =
		"SELECT uid "
		"FROM pending_email_confirmation "
		"WHERE confirmation_id="s + conn.quoteDontEscapeHtml (confirmId)
		+ " AND valid_until > CURRENT_TIMESTAMP;";
	auto result = conn.exec (checkIdQuery);
	if (result.size() == 0) return -1;

	int uid = result.at(0).at(0).as <int>();
	std::string removeId =
		+ "DELETE FROM pending_email_confirmation "
		+ "WHERE confirmation_id="s + conn.quoteDontEscapeHtml (confirmId);
	conn.exec (removeId);
	conn.commit();

	return uid;	
}

UserData UserManager::getUserDataById (int userId) {
	auto conn = database.connect ();
	return this->getUserDataWithWork (userId, conn);
}

UserData UserManager::getUserDataBySessionId (std::string sessionId) {
	if (sessionId == "") return UserData (false);
	auto conn = database.connect ();
	int uid = this->getUserIdBySessionId (sessionId, conn);
	return this->getUserDataWithWork (uid, conn);
}

/* static */ bool UserManager::isLoggedIn (std::string sessionId, int requiredAccessLevel) {
	if (sessionId == "") return false;
	auto &mgr = UserManager::get();
	return mgr.userIsAuthorized (sessionId, requiredAccessLevel);
}

bool UserManager::userIsAuthorized (std::string sessionId, int requiredAccessLevel) {
	if (sessionId == "") return false;
	auto conn = database.connect ();
	int userId = this->getUserIdBySessionId (sessionId, conn);
	return UserData (userId, conn).hasAccessLevel (requiredAccessLevel);
}

void UserManager::updateUserLastActionTime (std::string sessionId) {
	if (sessionId == "") return;
	auto conn = database.connect ();
	auto userId = this->getUserIdBySessionId (sessionId, conn);
	this->updateUserLastActionTimeWithWork (userId, conn);
}

void UserManager::updateUserLastActionTime (int userId) {
	auto conn = database.connect ();
	this->updateUserLastActionTimeWithWork (userId, conn);
}

bool UserManager::userExists (int id) {
	std::string query = "SELECT * FROM users WHERE uid="s + std::to_string (id) + ";";
	auto conn = database.connect ();
	return conn.exec (query).size () == 1;
}
