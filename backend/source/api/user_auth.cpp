
#include "user_auth.hpp"


UserLoginResource::UserLoginResource (mg_context* ctx, std::string uri):
ApiResource (ctx, uri) {

}

std::unique_ptr<ApiResponse> UserLoginResource::processRequest (RequestData &rd, nlohmann::json body) {
	if (rd.method != "POST") return makeApiResponse (405);

	std::string username = getParameter<std::string> ("username", body);
	std::string password = getParameter<std::string> ("password", body);
	std::string device_id = rd.ip;

	auto &mgr = UserManager::get();
	std::string userSession = mgr.loginUser (username, password, device_id);

	auto resp = makeApiResponse (nlohmann::json {{"status", "success"}}, 200);
	resp->setCookie (SESSION_ID, userSession, true);
	resp->setCookie ("username", username);

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
	auto response = makeApiResponse (nlohmann::json{{"status", "unauthorized"}}, 200);
	response->setCookie (SESSION_ID, "", true, 0);
	return response;
}

std::unique_ptr<ApiResponse> CheckSessionResource::processRequest (RequestData &rd, nlohmann::json body) {
	if (rd.method != "POST") return makeApiResponse (405);
	if (!rd.setCookies.contains (SESSION_ID)) return this->resetSessionId();

	auto &mgr = UserManager::get();
	auto user = mgr.getUserDataBySessionId (rd.setCookies [SESSION_ID]);
	if (!user.valid()) return this->resetSessionId();
	return makeApiResponse (nlohmann::json{{"status", "authorized"}, {"username", user.username()}, {"uid", user.id()}}, 200);
}
