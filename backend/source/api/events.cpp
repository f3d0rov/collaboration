
#include "events.hpp"


void to_json (nlohmann::json &j, const ParticipantEntity &pe) {
	j["created"] = pe.created;
	if (pe.created) {
		j["entity_id"] = pe.entityId;
		if (pe.name != "") j["name"] = pe.name;
	} else {
		j["name"] = pe.name;
	}
}

void from_json (const nlohmann::json &j, ParticipantEntity &pe) {
	pe.created = j["created"].get <bool>();
	if (pe.created) {
		pe.entityId = j["entity_id"].get <int>();
		if (j.contains ("name")) pe.name = j["name"].get <std::string>();
	} else {
		pe.name = j["name"].get <std::string>();
	}
}

void to_json (nlohmann::json &j, const EventData &ed) {
	j ["id"] = ed.id;
	
	j ["name"] = ed.name;
	j ["description"] = ed.desc;
	j ["type"] = ed.type;

	j ["start_date"] = ed.startDate;
	if (ed.endDate.has_value()) j ["end_date"] = ed.endDate.value();

	j ["participants"] = ed.participants;
}


CreateEntityEventResource::CreateEntityEventResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

void CreateEntityEventResource::addUserContributionWithWork (pqxx::work &work, const UsernameUid &user, int eventId) {
	std::string insertUserContributionQuery = "INSERT INTO user_event_contributions (user_id, contributed_on, event_id) VALUES ("
		+ /* user id */ 	work.quote (user.uid) + ","
		+ /* contrib on*/	"CURRENT_DATE,"
		+ /* event id */	std::to_string (eventId) + ");";
	work.exec0 (insertUserContributionQuery);
}

void CreateEntityEventResource::addParticipantWithWork (pqxx::work &work, const ParticipantEntity &pe, int eventId) {
	int entityId;
	if (pe.created) {
		entityId = pe.entityId;
	} else {
		entityId = EntityDataResource::getEntityByNameWithWork (work, pe.name);
	}
	std::string addParticipationQuery = "INSERT INTO participation (event_id, entity_id) VALUES ("s
		+ /* event id */ std::to_string (eventId) + ","
		+ /* entity id*/ std::to_string (entityId) + ");";
	work.exec (addParticipationQuery);
}

ApiResponsePtr CreateEntityEventResource::processRequest (RequestData &rd, nlohmann::json body) {
	UsernameUid user = CheckSessionResource::checkSessionId (rd);
	if (!user.valid) return makeApiResponse (401);
	if (!(
		body.contains ("name")
		&& body.contains ("desc")
		&& body.contains ("type")
		&& body.contains ("start_date")
		&& body.contains ("participants")
	)) return makeApiResponse (400);

	std::string name, desc, type, startDate;
	std::optional <std::string> endDate;
	std::vector <ParticipantEntity> participants;
	try {
		name = body ["name"].get <std::string>();
		desc = body ["desc"].get <std::string>();
		type = body ["type"].get <std::string>();
		startDate = body ["start_date"].get <std::string>();
		if (body.contains ("end_date")) endDate = body ["end_date"].get <std::string> ();
		participants = body ["participants"].get <std::vector <ParticipantEntity>>();
	} catch (nlohmann::json::exception &e) {
		return makeApiResponse (nlohmann::json{{"error", e.what()}}, 400);
	}

	auto conn = database.connect();
	pqxx::work work (*conn.conn);
	
	// Insert event ***************************************
	std::string insertEventQuery = 
		"INSERT INTO events (name, description, type, start_date, end_date) "s
		+ " VALUES ("
		+ /* name */ 			work.quote (name) + ","
		+ /* description */		work.quote (desc) + ","
		+ /* type */			work.quote (type) + ","
		+ /* start_date */		work.quote (startDate) + ","
		+ /* end_date */		(endDate.has_value() ? work.quote (endDate.value()) : "NULL")
		+ ") RETURNING id;";
	pqxx::row eventIdRow = work.exec1 (insertEventQuery);
	int eventId = eventIdRow[0].as <int>();

	// Insert user contribution ***************************
	CreateEntityEventResource::addUserContributionWithWork (work, user, eventId);

	// Insert participants ********************************
	for (const auto &i: participants) addParticipantWithWork (work, i, eventId);
	work.commit();

	return makeApiResponse (nlohmann::json {{"status", "success"}, {"event_id", eventId}}, 200);
}




GetEntityEventsResource::GetEntityEventsResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

std::vector <ParticipantEntity> GetEntityEventsResource::getEventParticipantsWithWork (pqxx::work &work, int eventId) {
	std::vector <ParticipantEntity> participants;
	std::string getParticipantsQuery = 
		"SELECT id, name, awaits_creation "s
		+ "FROM participation INNER JOIN entities on participation.entity_id=entities.id "
		+ "WHERE event_id=" + std::to_string (eventId) + ";";
	auto result = work.exec (getParticipantsQuery);
	for (int i = 0; i < result.size(); i++) {
		ParticipantEntity pe;
		pqxx::row row = result[i];
		pe.entityId = 	row[0].as <int>();
		pe.name = 		row[1].as <std::string>();
		pe.created = 	!(row[2].as <bool>());

		participants.push_back (pe);
	}

	return participants;
}

ApiResponsePtr GetEntityEventsResource::processRequest (RequestData &rd, nlohmann::json body) {
	if (rd.method != "POST") return makeApiResponse (405);
	// No auth required
	if (!body.contains ("entity_id")) return makeApiResponse (400);
	int entityId = 0;
	try {
		entityId = body["entity_id"].get <int>();
	} catch (nlohmann::json::exception &e) {
		return makeApiResponse (nlohmann::json{{"error", e.what()}}, 400);
	}

	std::string getEventsQuery = 
		"SELECT id, name, description, type, start_date, end_date "s
		+ "FROM participation INNER JOIN events ON participation.event_id = events.id "s
		+ "WHERE participation.entity_id = " + std::to_string (entityId) + ";";

	auto conn = database.connect();
	pqxx::work work (*conn.conn);

	pqxx::result eventsQueryResults = work.exec (getEventsQuery);
	if (eventsQueryResults.size() == 0) {
		return makeApiResponse (nlohmann::json{{"events", nlohmann::json::array()}}, 200);
	}
	
	std::vector <EventData> events;
	for (int i = 0; i < eventsQueryResults.size(); i++) {
		EventData ed;
		auto row = eventsQueryResults [i];

		ed.id = row[0].as <int>();
		ed.name = row[1].as <std::string>();
		ed.desc = row[2].as <std::string>();
		ed.type = row[3].as <std::string>();
		ed.startDate = row[4].as <std::string>();
		if (row[5].is_null() == false) ed.endDate = row[5].as <std::string>();
		
		events.push_back (ed);
	}

	for (auto &i: events) {
		i.participants = GetEntityEventsResource::getEventParticipantsWithWork (work, i.id);
	}

	return makeApiResponse (nlohmann::json{{"events", events}}, 200);
}


UpdateEntityEventResource::UpdateEntityEventResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

ApiResponsePtr UpdateEntityEventResource::processRequest (RequestData &rd, nlohmann::json body) {
	if (!CheckSessionResource::isLoggedIn (rd)) return makeApiResponse (401);
	
}




DeleteEntityEventResource::DeleteEntityEventResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

ApiResponsePtr DeleteEntityEventResource::processRequest (RequestData &rd, nlohmann::json body) {
	if (!CheckSessionResource::isLoggedIn (rd, 1)) return makeApiResponse (401);
	
}




ReportEntityEventResource::ReportEntityEventResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

ApiResponsePtr ReportEntityEventResource::processRequest (RequestData &rd, nlohmann::json body) {
	
}




GetEntityEventReportListResource::GetEntityEventReportListResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

ApiResponsePtr GetEntityEventReportListResource::processRequest (RequestData &rd, nlohmann::json body) {
	if (!CheckSessionResource::isLoggedIn (rd, 1)) return makeApiResponse (401);
	
}
