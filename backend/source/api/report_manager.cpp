
#include "report_manager.hpp"


std::unique_ptr <ReportManager> ReportManager::_manager;

ReportManager::ReportManager () {

}


/* static */ ReportManager &ReportManager::get () {
	// Create a `ReportManager` object if it doesn't exist yet
	if (ReportManager::_manager.get() == nullptr)
		ReportManager::_manager = std::unique_ptr <ReportManager> (new ReportManager);

	// Return a reference to the manager object
	return *ReportManager::_manager; 
}

void ReportManager::refreshReportReasons () {
	// Take unique ownership of the `_cacheMutex` to stop reading & writing from other threads
	std::unique_lock modifyingCache (this->_cacheMutex);
	
	// Do not update report reasons if another thread updated them while this thread waited for mutex ownership
	if (this->reportReasonCacheInvalid_unsafe() == false) return;

	// Connect to the database and execute the SELECT query
	const std::string query = "SELECT id, name, reportable_type FROM report_reasons;";
	auto conn = database.connect();
	auto res = conn.exec (query);

	// Construct a vector of `ReportReason` objects
	std::vector <ReportManager::ReportReason> reasons;
	for (int i = 0; i < res.size(); i++) {
		auto row = res[i];

		// Construct a `ReportReason` object for the given row
		ReportManager::ReportReason r;
		r.id = row.at ("id").as <int>();
		r.name = row.at ("name").as <std::string>();
		r.reportableType = row.at ("reportable_type").as <std::string>();

		// Push it to the vector
		reasons.push_back (r);
	}

	// Swap newly created vector with the old one
	std::swap (this->_reasonCache, reasons);
	// Set cache invalidation time
	this->_reasonCacheIsValidUntil = ReportManager::CacheTimepoint::clock::now() + this->_cacheValidPeriod;
}


bool ReportManager::reportReasonCacheInvalid_unsafe () const {
	// Cache is invalid if it is empty or if it is too old
	return this->_reasonCache.size() == 0 || (this->_reasonCacheIsValidUntil < ReportManager::CacheTimepoint::clock::now());
}


std::vector <ReportManager::ReportReason> ReportManager::getReportTypes () {
	// Lock cache for reading
	std::shared_lock readingCache (this->_cacheMutex);

	// Check if cache is invalid
	if (this->reportReasonCacheInvalid_unsafe()) {
		// Update report reasons from the database while unlocking the mutex so that `refreshReportReasons` could
		// get a unique ownership of the mutex in order to write data
		readingCache.unlock();
		this->refreshReportReasons();

		// Lock the cache again in order to copy it
		readingCache.lock();
	}

	// Return a copy of the stored cache
	return this->_reasonCache;
}


std::vector <ReportManager::ReportReason> ReportManager::getReportTypesFor (const std::string &reportableType) {
	// Make a copy of current cache of reasons
	std::vector <ReportManager::ReportReason> reasons = this->getReportTypes();
	std::vector <ReportManager::ReportReason> typeReasons;

	// Add each reason with needed `reportableType` to the `typeReasons` vector
	for (auto i: reasons) {
		if (i.reportableType == reportableType) typeReasons.push_back (i);
	}

	return typeReasons;	
}


void ReportManager::report (const ReportManager::ReportTicket &rt, int reportedBy) {
	// Form a query to add a report to db or do nothing if the user has already reported this object
	std::string query = "INSERT INTO reports (reported_id, reason_id, reported_by, reported_on) VALUES ("s
		+ std::to_string(rt.reportedId) + ","
		+ std::to_string (rt.reasonId) + ","
		+ std::to_string (reportedBy) + ", CURRENT_DATE) ON CONFLICT ON CONSTRAINT single_unique_report DO NOTHING;";

	// Connect to the database and execute the query
	auto conn = database.connect();
	conn.exec0 (query);
}


std::vector <ReportManager::ReportInfo> ReportManager::getPendingReports () {
	// Form a query to get info from the db
	const std::string query =
		"SELECT reported_id, reason_id, reported_by, report_id, reported_on "
		"FROM reports INNER JOIN report_reasons ON reports.reason_id = report_reasons.id"
		"WHERE pending=TRUE;";
	
	// Connect to the database and execute the query
	auto conn = database.connect();
	auto result = conn.exec (query);
	conn.release();

	// Initialize return data vector
	std::vector <ReportManager::ReportInfo> returnData;
	returnData.reserve (result.size());

	// Convert every row to a `ReportManager::ReportInfo` object
	for (int i = 0; i < result.size(); i++) {
		auto row = result [i];

		// Create a `ReportInfo` object and fill it with data from the row
		ReportManager::ReportInfo info;
		info.pending = true;
		info.reasonId = row.at ("reason_id").as <int>();
		info.reportedBy = row.at ("reported_by").as <int>();
		info.reportedId = row.at ("reported_id").as <int>();
		info.reportedOn = row.at ("reported_on").as <std::string>();
		info.reportId = row.at ("report_id").as <int>();

		// Push the object into the return vector
		returnData.push_back (info);
	}

	// Return the resulting vector
	return returnData;
}


void ReportManager::reportSatisfied (int reportId, int byUser) {
	// Get user manager and check user's access level
	auto &userMgr = UserManager::get();
	auto userData = userMgr.getUserDataById (byUser);

	// Return if user is not authorized to satisfy reports 
	if (userData.hasAccessLevel (1) == false) throw UserMistakeException ("Пользователь не уполномочен", 403, "forbidden");

	// Form a query to close the report
	const std::string query = 
		"UPDATE reports "
		"SET pending=FALSE,"
			"closed_by="s + std::to_string (byUser) + " "
		"WHERE report_id=" + std::to_string (reportId) + ";";

	// Connect to the database, start a transaction. Execute the query
	auto conn = database.connect();
	int affectedRows = conn.exec0 (query).affected_rows();

	// If `affectedRows` is 0, then the provided reportId is invalid
	if (affectedRows == 0) throw UserMistakeException ("Репорта с заданным id не существует", 404, "report_not_found");

	// More than one rows affected means that query logic is false and result should not be commited
	if (affectedRows > 1) throw std::logic_error ("ReportManager::reportSatisfied: affectedRows > 1");

	// 1 row affected: everything's OK, commit changes.
	conn.commit();
}





void to_json (nlohmann::json &j, const ReportManager::ReportReason &rt) {
	j ["reportable_type"] = rt.reportableType;
	j ["name"] = rt.name;
	j ["reason_id"] = rt.id;
}

void from_json (const nlohmann::json &j, ReportManager::ReportTicket &rt) {
	rt.reportedId = j.at ("reported_id").get <int> ();
	rt.reasonId = j.at ("reason_id").get <int>();
}

void to_json (nlohmann::json &j, const ReportManager::ReportInfo &rt) {
	j ["reported_id"] = rt.reportedId;
	j ["reason_id"] = rt.reasonId;

	j ["reported_by"] = rt.reportedBy;
	j ["report_id"] = rt.reportId;
	j ["reported_on"] = rt.reportedOn;
	j ["pending"] = rt.pending;
}




GetReportReasonsResource::GetReportReasonsResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

ApiResponsePtr GetReportReasonsResource::processRequest (RequestData &rd, nlohmann::json body) {
	assertMethod (rd, "GET");

	// Access the report manager and respond with the report types
	auto &mgr = ReportManager::get();
	return makeApiResponse (nlohmann::json {{"reasons", mgr.getReportTypes()}}, 200);
}


ReportResource::ReportResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

ApiResponsePtr ReportResource::processRequest (RequestData &rd, nlohmann::json body) {
	assertMethod (rd, "POST");

	// Access the user manager and check user's session data
	auto &userMgr = UserManager::get();
	auto user = userMgr.getUserDataBySessionId (rd.getCookie (SESSION_ID));

	// Respond 401 if user is not authorized
	if (user.valid() == false) makeApiResponse (nlohmann::json{{"status", "unauthorized"}}, 401);

	// Access the report manager and file a report ticket
	auto &reportMgr = ReportManager::get();
	auto ticket = getParameter <ReportManager::ReportTicket> ("ticket", body);
	reportMgr.report (ticket, user.id());

	// Everything's OK, return 200
	return makeApiResponse (nlohmann::json {{"status", "success"}}, 202);
}


GetReportsResource::GetReportsResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

ApiResponsePtr GetReportsResource::processRequest (RequestData &rd, nlohmann::json body) {
	assertMethod (rd, "GET");

	// Access the user manager and check user's permissions
	auto &userMgr = UserManager::get();
	auto user = userMgr.getUserDataBySessionId (rd.getCookie (SESSION_ID));
	
	// Respond 401 if user is unauthorized
	if (user.valid() == false)
		return makeApiResponse (nlohmann::json {{"status", "unauthorized"}}, 401);

	// Respond 403 if user doesn't have the permission to view this resource
	if (user.accessLevel() < 1)
		return makeApiResponse (nlohmann::json {{"status", "forbidden"}}, 403);

	// Access the report manager and respond with the pending reports
	auto &reportMgr = ReportManager::get();
	return makeApiResponse (nlohmann::json {{"reports", reportMgr.getPendingReports()}}, 200);
}




SatisfyReportResource::SatisfyReportResource (mg_context *ctx, std::string uri):
ApiResource (ctx, uri) {

}

ApiResponsePtr SatisfyReportResource::processRequest (RequestData &rd, nlohmann::json body) {
	assertMethod (rd, "POST");

	// Access the user manager and check user's permissions
	auto &userMgr = UserManager::get();
	auto user = userMgr.getUserDataBySessionId (rd.getCookie (SESSION_ID));
	
	// Respond 401 if user is unauthorized
	if (user.valid() == false)
		return makeApiResponse (nlohmann::json {{"status", "unauthorized"}}, 401);

	// Respond 403 if user doesn't have the permission to view this resource
	if (user.accessLevel() < 1)
		return makeApiResponse (nlohmann::json {{"status", "forbidden"}}, 403);

	// Get the id of the report that was satisfied
	int reportId = getParameter <int> ("report_id", body);

	// Access the report manager and set the report's status
	auto &reportMgr = ReportManager::get();
	reportMgr.reportSatisfied (reportId, user.id());

	// Everything's ok, respond 200
	return makeApiResponse (nlohmann::json{{"status", "success"}}, 200);
}
