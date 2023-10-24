#pragma once

#include "../civetweb/civetweb.h"
#include "../api_resource.hpp"
#include "../database.hpp"
#include "../utils.hpp"
#include "../randomizer.hpp"

#define SESSION_ID_LEN 128
#define SALT_LEN 64


class UserLoginResource;
class CheckUsernameAvailability;
class CheckEmailAvailability;
class UserRegisterResource;
class UserPublicDataResource;
struct UsernameUid;
class CheckSessionResource;

std::string hashForPassword (std::string password, std::string salt);

/*********************
 * POST {username, password} to attempt login
 * Returns:
 * Success -> {"username": username, "uid": uid , status: "success"}
 * No specified user -> {"status": "no_such_user"}
 * Incorrect password -> {"status": "incorrect_password"}
*/
class UserLoginResource: public ApiResource {
	private:

		ApiResponse successfulLogin (pqxx::work& work, int uid, std::string device_ip, std::string username);
	public:
		UserLoginResource (mg_context* ctx, std::string uri);

		static std::string generateUniqueSessionId (pqxx::work& work);
		static std::string authUserWithWork (int uid, std::string device_ip, pqxx::work &work);
		static std::string authUser (int uid, std::string device_ip);

		ApiResponse processRequest (RequestData &rd, nlohmann::json body) override;
};


/**********
 * POST {} -> clears user_login session
*/
class UserLogoutResource: public ApiResource {
	public:
		UserLogoutResource (mg_context* ctx, std::string uri);
		ApiResponse processRequest (RequestData& rd, nlohmann::json body) override;
};


/*********************
 * POST {username} to check username availability
 * Returns:
 * Free -> {"status": "free", "username": username},
 * Taken -> {"status": "taken", "username": username}
*/
class CheckUsernameAvailability: public ApiResource {
	public:
		CheckUsernameAvailability (mg_context* ctx, std::string uri);
		static bool isAvailable (std::string username);

		ApiResponse processRequest (RequestData &rd, nlohmann::json body) override;
};


/*********************
 * POST {email} to check email availability
 * Returns:
 * Free -> {"status": "free", "email": email},
 * Taken -> {"status": "taken", "email": email}
*/
class CheckEmailAvailability: public ApiResource {
	public:
		CheckEmailAvailability (mg_context* ctx, std::string uri);
		static bool isAvailable (std::string email);

		ApiResponse processRequest (RequestData &rd, nlohmann::json body) override;
};


/********************
 * POST {username, password, email, device_id} to attempt registration
 * Returns a login cookie on success
 * Possible responses: 
 * { {"username": username}, {"uid": uid}, {"status": "success"} }
 * {"status": "username_taken"}
 * {"status": "email_taken"} 
 * {"status", "incorrect_email_format"}
 * {"status", "incorrect_username_format"}
 * {"status", "incorrect_password_format"}
*/
class UserRegisterResource: public ApiResource {
	private:
		std::string generateSalt ();
		bool checkPasswordFormat (std::string password);
		bool checkUsernameFormat (std::string username);
		bool checkEmailFormat (std::string email);
	public:
		UserRegisterResource (mg_context* ctx, std::string uri);
		ApiResponse processRequest (RequestData &rd, nlohmann::json body) override;
};


/**********
 * GET uid -> user data (contributions, username, etc...)
*/
class UserPublicDataResource: public ApiResource {
	public:
		UserPublicDataResource (mg_context* ctx, std::string uri);
		ApiResponse processRequest (RequestData &rd, nlohmann::json body) override;
};


struct UsernameUid {
	std::string username;
	int uid;
	int permissionLevel;
	bool valid = false;
};


/*********
 * POST {} -> checks if http-only cookie session_id is set & valid, returns user id, username if true
*/
class CheckSessionResource: public ApiResource {
	public:
		CheckSessionResource (mg_context* ctx, std::string uri);
		static UsernameUid checkSessionId (std::string sessionId);
		ApiResponse processRequest (RequestData& rd, nlohmann::json body) override;
};
