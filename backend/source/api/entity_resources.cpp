
#include "entity_resources.hpp"


GetEntityTypesResource::GetEntityTypesResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

ApiResponsePtr GetEntityTypesResource::processRequest (RequestData &rd, nlohmann::json body) {
	assertMethod (rd, "GET");
	auto &mgr = EntityManager::get();
	return makeApiResponse (nlohmann::json {{"types", mgr.getAvailableTypes()}}, 200);
}


CreateEntityResource::CreateEntityResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {
	
}

std::unique_ptr<ApiResponse> CreateEntityResource::processRequest (RequestData &rd, nlohmann::json body) {
	assertMethod (rd, "POST");
	auto &userManager = UserManager::get();
	auto user = userManager.getUserDataBySessionId (rd.getCookie(SESSION_ID));
	if (!user.valid()) return makeApiResponse (nlohmann::json{}, 401);

	EntityManager::BasicEntityData data;
	try {
		data = body.get <EntityManager::BasicEntityData> ();
	} catch (nlohmann::json::exception &e) {
		throw UserMistakeException ("Невозможно преобразовать данные к необходимому типу", 400, "bad_json");
	}

	auto &mgr = EntityManager::get();

	int entityId = mgr.createEntity (data, user.id());
	std::string url = EntityManager::urlForEntity (entityId);

	auto response = makeApiResponse (
		nlohmann::json {
			{"status", "success"},
			{"entity_id", entityId},
			{"url", url}
		},
		201
	);
	return response;
}




GetEntityResource::GetEntityResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

ApiResponsePtr GetEntityResource::processRequest (RequestData &rd, nlohmann::json body) {
	assertMethod (rd, "POST");

	int id = getParameter <int> ("entity_id", body);
	auto &mgr = EntityManager::get();

	auto data = mgr.getEntity (id);
	auto response = makeApiResponse (200);
	response->body = data;

	return response;
}




RequestEntityPictureChangeResource::RequestEntityPictureChangeResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

ApiResponsePtr RequestEntityPictureChangeResource::processRequest (RequestData &rd, nlohmann::json body) {
	assertMethod (rd, "POST");

	auto &userManager = UserManager::get();
	auto user = userManager.getUserDataBySessionId (rd.getCookie(SESSION_ID));
	if (!user.valid()) return makeApiResponse (nlohmann::json{}, 401);

	assertMethod (rd, "POST");
	int entityId = getParameter <int> ("entity_id", body);
	auto &mgr = EntityManager::get();
	std::string uploadUrl = mgr.getUrlToUploadPicture (entityId);
	return makeApiResponse (nlohmann::json {{"url", uploadUrl}});
}



