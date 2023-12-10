
#include "user_auth.hpp"


UserLoginResource::UserLoginResource (mg_context* ctx, std::string uri):
ApiResource (ctx, uri) {

}

std::unique_ptr<ApiResponse> UserLoginResource::processRequest (RequestData &rd, nlohmann::json body) {
	assertMethod (rd, "POST");

	// Get parameters that were sent to the api
	std::string username = getParameter<std::string> ("username", body);
	std::string password = getParameter<std::string> ("password", body);
	std::string device_id = rd.ip;

	// Access the user manager and login user if provided username and password are correct
	auto &mgr = UserManager::get();
	std::string userSession = mgr.loginUser (username, password, device_id);

	// Get user data to check their access level
	auto user = mgr.getUserDataBySessionId (userSession);

	// Success! Form a response
	auto resp = makeApiResponse (nlohmann::json {{"status", "success"}}, 200);

	// Set session id, username cookies
	resp->setCookie (SESSION_ID, userSession, true);
	resp->setCookie ("username", username);

	// Set the access level cookie. Its only puprpose is to make sure that a user without proper
	// permissions doesn't see the UI elements that they are not able to interact with.
	resp->setCookie ("access_level", std::to_string (user.accessLevel()));

	return resp;
}



UserLogoutResource::UserLogoutResource (mg_context* ctx, std::string uri):
ApiResource (ctx, uri) {

}

std::unique_ptr<ApiResponse> UserLogoutResource::processRequest (RequestData& rd, nlohmann::json body) {
	if (rd.method != "POST") return makeApiResponse (405);
	if (!rd.setCookies.contains (SESSION_ID)) return makeApiResponse (400);
	std::string session_id = rd.setCookies[SESSION_ID];
	auto &mgr = UserManager::get();
	mgr.logoutUser (session_id);
	return makeApiResponse (200);
}




CheckUsernameAvailability::CheckUsernameAvailability (mg_context* ctx, std::string uri):
ApiResource (ctx, uri) {

}

std::unique_ptr<ApiResponse> CheckUsernameAvailability::processRequest(RequestData &rd, nlohmann::json body) {
	if (rd.method != "POST") return makeApiResponse (405);
	std::string username = getParameter <std::string> ("username", body);

	auto &mgr = UserManager::get();

	auto response = makeApiResponse (200);
	response->body["username"] = username;
	response->body["status"] = mgr.usernameIsAvailable (username) ? "free" : "taken";

	return response;
}



CheckEmailAvailability::CheckEmailAvailability (mg_context* ctx, std::string uri):
ApiResource (ctx, uri) {

}

std::unique_ptr<ApiResponse> CheckEmailAvailability::processRequest(RequestData &rd, nlohmann::json body) {
	if (rd.method != "POST") return makeApiResponse (405);
	std::string email = getParameter <std::string> ("email", body);

	auto &mgr = UserManager::get();

	auto response = makeApiResponse (200);
	response->body["email"] = email;
	response->body["status"] = mgr.emailIsAvailable (email) ? "free" : "taken";

	return response;
}



UserRegisterResource::UserRegisterResource (mg_context* ctx, std::string uri):
ApiResource (ctx, uri) {

}

std::unique_ptr<ApiResponse> UserRegisterResource::processRequest (RequestData &rd, nlohmann::json body){
	if (rd.method != "POST") return makeApiResponse (405);
	std::string username, email, password;
	username = getParameter <std::string> ("username", body);
	email = getParameter <std::string> ("email", body);
	password = getParameter <std::string> ("password", body);
	
	auto &mgr = UserManager::get();
	mgr.registerUser (username, password, email);

	return makeApiResponse (nlohmann::json{ {"status", "success"} }, 200);
}



ConfirmRegistrationResource::ConfirmRegistrationResource (mg_context* ctx, std::string uri):
Resource (ctx, uri) {
	logger << "Интерфейс API: \"" + uri + "\"" << std::endl;
}

std::unique_ptr<_Response> ConfirmRegistrationResource::processRequest (RequestData &rd) {
	auto response = makeApiResponse (200);
	response->redirect(common.http + "://" + common.domain + "/");
	
	if (!rd.query.contains ("id")) return response;
	std::string id = rd.query["id"];

	auto &mgr = UserManager::get(); 
	int uid = mgr.confirmUserEmail (id);
	
	if (uid < 0) {
		response->status = 404;
		return response;
	}

	std::string sessionId = mgr.authUser (uid, rd.ip);
	std::string username = mgr.getUserDataById (uid).username();
	
	response->setCookie (SESSION_ID, sessionId, true);
	response->setCookie ("username", username, false);

	return response;
}


CheckSessionResource::CheckSessionResource (mg_context* ctx, std::string uri):
ApiResource (ctx, uri) {

}

ApiResponsePtr CheckSessionResource::resetSessionId () {
	// Return 200 because it is a normal result for this resource even though the user is unauthorized
	auto response = makeApiResponse (nlohmann::json{{"status", "unauthorized"}}, 200);
	response->setCookie (SESSION_ID, "", true, 0);
	response->setCookie ("access_level", "", true, 0);
	return response;
}

std::unique_ptr<ApiResponse> CheckSessionResource::processRequest (RequestData &rd, nlohmann::json body) {
	assertMethod (rd, "POST");
	
	// Reset user's cookies if he did not provide a session
	if (!rd.setCookies.contains (SESSION_ID)) return this->resetSessionId();

	// Access the user manager and get the user's data
	auto &mgr = UserManager::get();
	auto user = mgr.getUserDataBySessionId (rd.setCookies [SESSION_ID]);

	// Reset user's cookies if his session id is not valid
	if (!user.valid()) return this->resetSessionId();

	// Return user info if he is properly authorized
	return makeApiResponse (nlohmann::json{{"status", "authorized"}, {"username", user.username()}, {"uid", user.id()}, {"access_level", user.accessLevel()}}, 200);
}
