
#include "event_resources.hpp"


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

ApiResponsePtr CreateEntityEventResource::processRequest (RequestData &rd, nlohmann::json body) {
	if (rd.method != "POST") return makeApiResponse (405);

	UserManager &mgr = UserManager::get();
	EventManager &eventManager = EventManager::getManager();

	UserData user = mgr.getUserDataBySessionId (rd.getCookie (SESSION_ID));
	if (!user.valid()) return makeApiResponse (401);

	try {
		int createdEventId = eventManager.createEvent (body, user.id());
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
	if (rd.method != "POST") return makeApiResponse (405);
	if (!body.contains ("id")) return makeApiResponse (nlohmann::json{{"error", "Отсутствует поле id"}}, 400);
	

	int entityId = getParameter <int> ("id", body);

	EventManager &eventManager = EventManager::getManager();

	try {
		return makeApiResponse (eventManager.getEventsForEntity (entityId), 200);
	} catch (UserSideEventException &e) {
		return makeApiResponse (nlohmann::json {{"error", e.what()}}, 400);
	} catch (std::exception &e) {
		logger << "GetEntityEventsResource::processRequest: " << e.what() << std::endl;
		return makeApiResponse (500);
	}
}


UpdateEntityEventResource::UpdateEntityEventResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

ApiResponsePtr UpdateEntityEventResource::processRequest (RequestData &rd, nlohmann::json body) {
	if (rd.method != "POST") return makeApiResponse (405);

	UserManager &userManager = UserManager::get();
	EventManager &eventManager = EventManager::getManager();
	UserData user = userManager.getUserDataBySessionId (rd.getCookie (SESSION_ID));

	if (!user.valid()) return makeApiResponse (401);


	try {
		int updatedEvent = eventManager.updateEvent (body, user.id());
		return makeApiResponse (eventManager.getEvent (updatedEvent), 200);
	} catch (UserSideEventException &e) {
		return makeApiResponse (nlohmann::json {{"error", e.what()}}, 400);
	} catch (std::exception &e) {
		logger << "UpdateEntityEventResource::processRequest: " << e.what() << std::endl;
		return makeApiResponse (500);
	}
}




DeleteEntityEventResource::DeleteEntityEventResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

ApiResponsePtr DeleteEntityEventResource::processRequest (RequestData &rd, nlohmann::json body) {
	if (rd.method != "POST") return makeApiResponse (405);

	UserManager &userManager = UserManager::get();
	EventManager &eventManager = EventManager::getManager();
	UserData user = userManager.getUserDataBySessionId (rd.getCookie (SESSION_ID));

	if (!user.valid()) return makeApiResponse (401);
	if (!user.hasAccessLevel(1)) return makeApiResponse (401);

	try {
		eventManager.deleteEvent (body, user.id());
		return makeApiResponse (nlohmann::json{{"status", "success"}}, 200);
	} catch (UserSideEventException &e) {
		return makeApiResponse (nlohmann::json {{"error", e.what()}}, 400);
	} catch (std::exception &e) {
		logger << "DeleteEntityEventResource::processRequest: " << e.what() << std::endl;
		return makeApiResponse (500);
	}
}




ReportEntityEventResource::ReportEntityEventResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

ApiResponsePtr ReportEntityEventResource::processRequest (RequestData &rd, nlohmann::json body) {
	if (rd.method != "POST") return makeApiResponse (405);

	UserManager &userManager = UserManager::get();
	EventManager &eventManager = EventManager::getManager();
	UserData user = userManager.getUserDataBySessionId (rd.getCookie (SESSION_ID));
	
	if (!user.valid()) return makeApiResponse (401);

	int eventId, reasonId;
	eventId = getParameter <int> ("event_id", body);
	reasonId = getParameter <int> ("reason_id", body);

	try {
		eventManager.reportEvent (eventId, reasonId, user.id());
	} catch (UserSideEventException &e) {
		return makeApiResponse (nlohmann::json{{"error", e.what()}});
	}
	
	return makeApiResponse (nlohmann::json{{"status", "success"}}, 200);
}




// GetEntityEventReportListResource::GetEntityEventReportListResource (mg_context *ctx, std::string uri):
// ApiResource (ctx, uri) {

// }

// ApiResponsePtr GetEntityEventReportListResource::processRequest (RequestData &rd, nlohmann::json body) {
	
// }




EventReportTypesResource::EventReportTypesResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

ApiResponsePtr EventReportTypesResource::processRequest (RequestData &rd, nlohmann::json body) {
	if (rd.method != "GET") return makeApiResponse (405);
	EventManager &eventManager = EventManager::getManager();
	auto types = eventManager.getEventReportReasons ();
	return makeApiResponse (types, 200);
}

