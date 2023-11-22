
#include "eventResources.hpp"


GetEntityEventDescriptorsResource::GetEntityEventDescriptorsResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

ApiResponsePtr GetEntityEventDescriptorsResource::processRequest (RequestData &rd, nlohmann::json body) {
	if (rd.method != "GET") return makeApiResponse (405);
	EventManager &eventManager = EventManager::getManager ();
	return makeApiResponse (nlohmann::json {{"options", eventManager.getAvailableEventDescriptors ()}}, 200);
}




CreateEntityEventResource::CreateEntityEventResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

// void CreateEntityEventResource::addUserContributionWithWork (pqxx::work &work, const UsernameUid &user, int eventId) {
// 	std::string insertUserContributionQuery = "INSERT INTO user_event_contributions (user_id, contributed_on, event_id) VALUES ("
// 		+ /* user id */ 	work.quote (user.uid) + ","
// 		+ /* contrib on*/	"CURRENT_DATE,"
// 		+ /* event id */	std::to_string (eventId) + ");";
// 	work.exec0 (insertUserContributionQuery);
// }

ApiResponsePtr CreateEntityEventResource::processRequest (RequestData &rd, nlohmann::json body) {
	UsernameUid user = CheckSessionResource::checkSessionId (rd);
	if (!user.valid) return makeApiResponse (401);

	EventManager &eventManager = EventManager::getManager();

	try {
		int createdEventId = eventManager.createEvent (body, user.uid);
		return makeApiResponse (eventManager.getEvent (createdEventId), 200);
	} catch (UserSideEventException &e) {
		return makeApiResponse (nlohmann::json {{"error", e.what()}}, 400);
	} catch (std::exception &e) {
		logger << "CreateEntityEventResource::processRequest: " << e.what() << std::endl;
		return makeApiResponse (500);
	}
}




GetEntityEventsResource::GetEntityEventsResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

ApiResponsePtr GetEntityEventsResource::processRequest (RequestData &rd, nlohmann::json body) {
	if (!body.contains ("id")) return makeApiResponse (nlohmann::json{{"error", "Отсутствует поле id"}}, 400);
	
	int entityId;
	try {
		entityId = body["id"].get <int>();
	} catch (nlohmann::json::exception &e) {
		return makeApiResponse (nlohmann::json{{"error", e.what()}}, 400);
	}

	EventManager &eventManager = EventManager::getManager();

	try {
		return makeApiResponse (eventManager.getEventsForEntity (entityId), 200);
	} catch (UserSideEventException &e) {
		return makeApiResponse (nlohmann::json {{"error", e.what()}}, 400);
	} catch (std::exception &e) {
		logger << "CreateEntityEventResource::processRequest: " << e.what() << std::endl;
		return makeApiResponse (500);
	}
}


UpdateEntityEventResource::UpdateEntityEventResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

ApiResponsePtr UpdateEntityEventResource::processRequest (RequestData &rd, nlohmann::json body) {
	if (!CheckSessionResource::isLoggedIn (rd)) return makeApiResponse (401);
	UsernameUid user = CheckSessionResource::checkSessionId (rd);
	if (!user.valid) return makeApiResponse (401);

	EventManager &eventManager = EventManager::getManager();

	try {
		int updatedEvent = eventManager.updateEvent (body, user.uid);
		return makeApiResponse (eventManager.getEvent (updatedEvent), 200);
	} catch (UserSideEventException &e) {
		return makeApiResponse (nlohmann::json {{"error", e.what()}}, 400);
	} catch (std::exception &e) {
		logger << "CreateEntityEventResource::processRequest: " << e.what() << std::endl;
		return makeApiResponse (500);
	}
}




DeleteEntityEventResource::DeleteEntityEventResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

ApiResponsePtr DeleteEntityEventResource::processRequest (RequestData &rd, nlohmann::json body) {
	UsernameUid user = CheckSessionResource::checkSessionId (rd);
	if (user.permissionLevel < 1) return makeApiResponse (401);
	if (!user.valid) return makeApiResponse (401);

	EventManager &eventManager = EventManager::getManager();

	try {
		eventManager.deleteEvent (body, user.uid);
		return makeApiResponse (nlohmann::json{{"status", "success"}}, 200);
	} catch (UserSideEventException &e) {
		return makeApiResponse (nlohmann::json {{"error", e.what()}}, 400);
	} catch (std::exception &e) {
		logger << "CreateEntityEventResource::processRequest: " << e.what() << std::endl;
		return makeApiResponse (500);
	}
}




ReportEntityEventResource::ReportEntityEventResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

ApiResponsePtr ReportEntityEventResource::processRequest (RequestData &rd, nlohmann::json body) {
	if (rd.method != "POST") return makeApiResponse (405);
	auto user = CheckSessionResource::checkSessionId (rd);
	if (!user.valid) return makeApiResponse (401);

	int eventId, reasonId;
	try {
		eventId = body.at ("event_id").get <int>();
		reasonId = body.at ("reason_id").get <int>();
	} catch (nlohmann::json::exception &e) {
		return makeApiResponse (nlohmann::json{{"error", e.what()}}, 400);
	}

	EventManager &eventManager = EventManager::getManager();
	try {
		eventManager.reportEvent (eventId, reasonId, user.uid);
	} catch (UserSideEventException &e) {
		return makeApiResponse (nlohmann::json{{"error", e.what()}});
	}
	
	return makeApiResponse (nlohmann::json{{"status", "success"}}, 200);
}




GetEntityEventReportListResource::GetEntityEventReportListResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

ApiResponsePtr GetEntityEventReportListResource::processRequest (RequestData &rd, nlohmann::json body) {
	if (!CheckSessionResource::isLoggedIn (rd, 1)) return makeApiResponse (401);
	
}




EventReportTypesResource::EventReportTypesResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

ApiResponsePtr EventReportTypesResource::processRequest (RequestData &rd, nlohmann::json body) {
	if (rd.method != "GET") return makeApiResponse (405);
	EventManager &eventManager = EventManager::getManager();
	auto types = eventManager.getEventReportReasons ();

	return makeApiResponse (types, 200);
}

