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


std::string hashForPassword (std::string password, std::string salt);

/*********************
 * POST {username, password, device id} to attempt login
 * Returns:
 * Success -> {"session_id": sessionId}, "username": username, "uid": uid }
 * No specified user -> {"status": "no_such_user"}
 * Incorrect password -> {"status": "incorrect_password"}
*/
class UserLoginResource: public ApiResource {
	private:

		ApiResponse successfulLogin (pqxx::work& work, int uid, std::string device_id, std::string username);
	public:
		UserLoginResource (mg_context* ctx, std::string uri);

		static std::string generateUniqueSessionId (pqxx::work& work);
		static std::string authUserWithWork (int uid, std::string device_id, pqxx::work &work);
		static std::string authUser (int uid, std::string device_id);

		ApiResponse processRequest(std::string method, std::string uri, nlohmann::json body) override;
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

		ApiResponse processRequest(std::string method, std::string uri, nlohmann::json body) override;
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

		ApiResponse processRequest(std::string method, std::string uri, nlohmann::json body) override;
};


/********************
 * POST {username, password, email, device_id} to attempt registration
 * Returns a login cookie on success
 * Possible responses: 
 * { {"session_id": sessionId}, {"username": username}, {"uid": uid}, {"status": "success"} }
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
		ApiResponse processRequest(std::string method, std::string uri, nlohmann::json body) override;
};


/**********
 * GET uid -> user data (contributions, username, etc...)
*/
class UserPublicDataResource: public ApiResource {
	public:
		UserPublicDataResource (mg_context* ctx, std::string uri);
		ApiResponse processRequest(std::string method, std::string uri, nlohmann::json body) override;
};