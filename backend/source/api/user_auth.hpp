#pragma once

#include "../web_resource.hpp"
#include "../api_resource.hpp"
#include "../utils.hpp"
#include "user_manager.hpp"


class UserLoginResource;
class CheckUsernameAvailability;
class CheckEmailAvailability;
class UserRegisterResource;
class ConfirmRegistrationResource;
class UserPublicDataResource;
struct UsernameUid;
class CheckSessionResource;



/*********************
 * POST {username, password} to attempt login
 * Returns:
 * Success -> {status: "success"} and a confirmation link is sent to the specified email.
 * No specified user -> {"status": "no_such_user"}
 * Incorrect password -> {"status": "incorrect_password"}
*/
class UserLoginResource: public ApiResource {
	public:
		UserLoginResource (mg_context* ctx, std::string uri);
		std::unique_ptr<ApiResponse> processRequest (RequestData &rd, nlohmann::json body) override;
};


/**********
 * POST {} -> clears user_login session
*/
class UserLogoutResource: public ApiResource {
	public:
		UserLogoutResource (mg_context* ctx, std::string uri);
		std::unique_ptr<ApiResponse> processRequest (RequestData& rd, nlohmann::json body) override;
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
		std::unique_ptr<ApiResponse> processRequest (RequestData &rd, nlohmann::json body) override;
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
		std::unique_ptr<ApiResponse> processRequest (RequestData &rd, nlohmann::json body) override;
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
	public:
		UserRegisterResource (mg_context* ctx, std::string uri);
		std::unique_ptr<ApiResponse> processRequest (RequestData &rd, nlohmann::json body) override;
};


class ConfirmRegistrationResource: public Resource {
	public:
		ConfirmRegistrationResource (mg_context* ctx, std::string uri);
		std::unique_ptr<_Response> processRequest (RequestData &rd) override;
};

/**********
 * GET uid -> user data (contributions, username, etc...)
*/
class UserPublicDataResource: public ApiResource {
	public:
		UserPublicDataResource (mg_context* ctx, std::string uri);
		std::unique_ptr<ApiResponse> processRequest (RequestData &rd, nlohmann::json body) override;
};

/*********
 * POST {} -> checks if http-only cookie session_id is set & valid, returns user id, username if true
*/
class CheckSessionResource: public ApiResource {
	public:
		CheckSessionResource (mg_context* ctx, std::string uri);
		ApiResponsePtr resetSessionId ();
		std::unique_ptr<ApiResponse> processRequest (RequestData& rd, nlohmann::json body) override;
};
