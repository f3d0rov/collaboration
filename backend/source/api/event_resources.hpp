#pragma once

#include "../api_resource.hpp"

#include "events.hpp"
#include "user_auth.hpp"
#include "page.hpp"


class GetEntityEventDescriptorsResource;

class CreateEntityEventResource;
class GetEntityEventsResource;
class UpdateEntityEventResource;
class DeleteEntityEventResource;

class ReportEntityEventResource;
class GetEntityEventReportListResource;




class GetEntityEventDescriptorsResource: public ApiResource {
	public:
		GetEntityEventDescriptorsResource (mg_context *ctx, std::string uri);
		ApiResponsePtr processRequest (RequestData &rd, nlohmann::json body) override;
};


/******
 * POST {
 * 		"type": type,
 * 		"description": desc,
 * 		"data": { data according to type's descriptor }
 * 		"participants": [
 * 			{
 * 				"created": true,
 * 				"entity_id": entity id (int)
 * 			},
 * 			{
 * 				"created": false,
 * 				"name": band/person name (string)
 * 			}
 * 		]
 * } ->
 * {
 * 		"status": "success",
 * 		"event_id": created event id
 * }
*/
class CreateEntityEventResource: public ApiResource {
	public:
		CreateEntityEventResource (mg_context *ctx, std::string uri);
		// static void addUserContributionWithWork (pqxx::work &work, const UsernameUid &user, int eventId);
		ApiResponsePtr processRequest (RequestData &rd, nlohmann::json body) override;
};


/***********
 * POST {
 * 		"entity_id": entity id
 * } -> {
 * 		"events": [
 * 			{
 * 				"id": id
 * 				"type": type,
 * 				"sort_date": date,
 * 				"description": desc,
 * 
 * 				"data": {}
 * 				"participants": [
 * 					{
 * 						"created": true,
 * 						"entity_id": id,
 * 						"name": name
 * 					},
 * 					{
 * 						"created": false,
 * 						"name": name
 * 					}
 * 				]	
 * 			}
 * 		]
 * }
*/
class GetEntityEventsResource: public ApiResource {
	public:
		GetEntityEventsResource (mg_context *ctx, std::string uri);
		ApiResponsePtr processRequest (RequestData &rd, nlohmann::json body) override;
};


class UpdateEntityEventResource: public ApiResource {
	public:
		UpdateEntityEventResource (mg_context *ctx, std::string uri);
		ApiResponsePtr processRequest (RequestData &rd, nlohmann::json body) override;
};


class DeleteEntityEventResource: public ApiResource {
	public:
		DeleteEntityEventResource (mg_context *ctx, std::string uri);
		ApiResponsePtr processRequest (RequestData &rd, nlohmann::json body) override;
};


class ReportEntityEventResource: public ApiResource {
	public:
		ReportEntityEventResource (mg_context *ctx, std::string uri);
		ApiResponsePtr processRequest (RequestData &rd, nlohmann::json body) override;
};


// class GetEntityEventReportListResource : public ApiResource {
// 	public:
// 		GetEntityEventReportListResource (mg_context *ctx, std::string uri);
// 		ApiResponsePtr processRequest (RequestData &rd, nlohmann::json body) override;
// };

class EventReportTypesResource: public ApiResource {
	public:
		EventReportTypesResource (mg_context *ctx, std::string uri);
		ApiResponsePtr processRequest (RequestData &rd, nlohmann::json body) override;
};
