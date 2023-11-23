
#pragma once

#include <map>
#include <vector>

#include "../nlohmann-json/json.hpp"

#include "page.hpp"
#include "../database.hpp"


class UserSideEventException;
struct ParticipantEntity;
class InputTypeDescriptor;

class EventType;
class AllEntitiesEventType;
class BandOnlyEventType;
class PersonOnlyEventType;

class EventManager;



class UserSideEventException: public std::runtime_error {
	public:
		UserSideEventException (std::string w);
};


struct ParticipantEntity {
	bool created = false;
	int entityId;
	std::string name;

	void updateId ();
};

void to_json (nlohmann::json &j, const ParticipantEntity &pe);
void from_json (const nlohmann::json &j, ParticipantEntity &pe);


template <>
class UpdateQueryElement <ParticipantEntity>: public UpdateQueryElement <int> {
	public:
		UpdateQueryElement (std::string jsonElementName, std::string sqlColName, std::string sqlTableName);
		static UQEI_ptr make (std::string jsonElementName, std::string sqlColName, std::string sqlTableName);
		
		std::string getWrappedValue (const nlohmann::json &value, pqxx::work &w) override;
};


struct InputTypeDescriptor {
	std::string id;
	std::string prompt;
	std::string type;
	bool optional = false;

	int order = 0;

	InputTypeDescriptor ();
	InputTypeDescriptor (std::string id, std::string prompt, std::string type, bool optional = false);
};

void to_json (nlohmann::json &j, const InputTypeDescriptor &pe);


class EventType {
	protected:
		virtual ~EventType ();

		template <class T> T getParameter (std::string name, nlohmann::json &data);

		std::vector <ParticipantEntity> getParticipantsFromJson (nlohmann::json &data);
		void addParticipants (const int eventId, const std::vector <ParticipantEntity> &participants, pqxx::work &work);
		std::vector <ParticipantEntity> getParticipantsForEvent (int eventId, pqxx::work &work);
	
		nlohmann::json formGetEventResponse (pqxx::work &work, int eventId, std::string desc, int sortIndex, std::string startDate, nlohmann::json &data);
		nlohmann::json formGetEventResponse (pqxx::work &work, int eventId, std::string desc, int sortIndex, std::string startDate, std::string endDate, nlohmann::json &data);
	
	public:
		virtual std::string getTypeName () const = 0;
		virtual std::string getDisplayName () const = 0;
		virtual std::string getTitleFormat () const = 0;
		
		virtual std::vector <InputTypeDescriptor> getInputs () const = 0;
		virtual std::vector <std::string> getApplicableEntityTypes () const = 0;

		virtual nlohmann::json getEventDescriptor ();

		virtual int createEvent (nlohmann::json &data) = 0;
		virtual nlohmann::json getEvent (int id) = 0;
		virtual int updateEvent (nlohmann::json &data) = 0;
		virtual void deleteEvent (int id); // Default implementation deletes corresponding row in events table
};


class AllEntitiesEventType: virtual public EventType {
	public:
		std::vector <std::string> getApplicableEntityTypes () const final;
};


class BandOnlyEventType: virtual public EventType {
	public:
		std::vector <std::string> getApplicableEntityTypes () const final;
};


class PersonOnlyEventType: virtual public EventType {
	public:
		std::vector <std::string> getApplicableEntityTypes () const final;
};




class EventManager {
	private:
		static EventManager *_obj;
		EventManager ();
		EventManager (const EventManager &) = delete;
		EventManager (EventManager &&) = delete;
	
		std::map <std::string, std::shared_ptr <EventType>> _types;

		void addUserEventContribution (int userId, int eventId);

	public:
		struct EventReportReason {
			int id;
			std::string name;
		};

		struct EventReportCount {
			int reportTypeId;
			int count;
		};
		
		struct EventReportData {
			int eventId;
			std::vector <EventReportCount> statistics;
		};
	

		static EventManager &getManager ();
	
		void registerEventType (std::shared_ptr <EventType> et);
		int size() const;

		std::shared_ptr <EventType> getEventTypeByName (std::string typeName);
		std::shared_ptr <EventType> getEventTypeFromJson (nlohmann::json &data);
		std::shared_ptr <EventType> getEventTypeById (int eventId);

		// returns event id
		int createEvent (nlohmann::json &data, int byUser);
		nlohmann::json getEvent (int eventId);
		nlohmann::json getEventsForEntity (int entityId);
		int updateEvent (nlohmann::json &data, int byUser);
		void deleteEvent (int eventId, int byUser);

		// Data fields to create different event types
		nlohmann::json getAvailableEventDescriptors ();
		
		std::vector <EventReportReason> getEventReportReasons ();
		void reportEvent (int id, int reasonId, int byUser);
		std::vector <EventReportData> getEventReports ();
};

void to_json (nlohmann::json &j, const EventManager::EventReportReason &evrr);
void to_json (nlohmann::json &j, const EventManager::EventReportCount &evrc);
void to_json (nlohmann::json &j, const EventManager::EventReportData &evrd);

#include "events.tcc"
