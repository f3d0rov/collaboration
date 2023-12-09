#pragma once

#include <memory>
#include <shared_mutex>
#include <chrono>

#include "../database.hpp"
#include "../api_resource.hpp"


class ReportManager {
	public:
		struct ReportReason {
			std::string reportableType;
			std::string name;
			int id;
		};

		struct ReportTicket {
			int reportedId;
			int reasonId;
		};

		struct ReportInfo: public ReportTicket {
			int reportId;
			int reportedBy;
			std::string reportedOn;
			bool pending;
		};

		typedef std::chrono::time_point <std::chrono::system_clock> CacheTimepoint;

	private:
		static std::unique_ptr <ReportManager> _manager;
		ReportManager ();

		const ReportManager::CacheTimepoint::clock::duration _cacheValidPeriod = std::chrono::minutes (15);
		std::vector <ReportManager::ReportReason> _reasonCache;
		ReportManager::CacheTimepoint _reasonCacheIsValidUntil;
		std::shared_mutex _cacheMutex;

		void refreshReportReasons ();
		bool reportReasonCacheInvalid_unsafe () const; // Lock the `_cacheMutex` before calling

	public:
		ReportManager (const ReportManager &) = delete;
		ReportManager (ReportManager &&) = delete;
		static ReportManager &get ();

		std::vector <ReportManager::ReportReason> getReportTypes ();
		std::vector <ReportManager::ReportReason> getReportTypesFor (const std::string &reportableType);
		void report (const ReportManager::ReportTicket &rt, int reportedBy);
		std::vector <ReportManager::ReportInfo> getPendingReports ();
};

void to_json (nlohmann::json &j, const ReportManager::ReportReason &rt);
void from_json (const nlohmann::json &j, ReportManager::ReportTicket &rt);
void to_json (nlohmann::json &j, const ReportManager::ReportInfo &rt);
