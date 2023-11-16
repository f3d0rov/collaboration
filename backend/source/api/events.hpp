
#pragma once

#include <map>
#include <vector>

#include "../nlohmann-json/json.hpp"

#include "page.hpp"
#include "../database.hpp"


class UserSideEventException;
class ParticipantEntity;
class EventType;
class EventManager;

class BandFoundationEventType;


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


struct InputTypeDescriptor {
	std::string id;
	std::string prompt;
	std::string type;
	bool optional = false;

	InputTypeDescriptor ();
	InputTypeDescriptor (std::string id, std::string prompt, std::string type, bool optional = false);

	operator nlohmann::json ();
};

class EventType {
	protected:
		template <class T>
		T getParameter (std::string name, nlohmann::json &data);
		std::vector <ParticipantEntity> getParticipants (nlohmann::json &data);

	public:
		virtual std::string getTypeName () const = 0;
		virtual std::vector <InputTypeDescriptor> getInputs () const = 0;
		virtual std::vector <std::string> getApplicableEntityTypes () const = 0;

		virtual nlohmann::json getEventDescriptor ();

		virtual int createEvent (nlohmann::json &data) = 0;
		virtual nlohmann::json getEvent (int id) = 0;
		virtual void updateEvent (nlohmann::json &data) = 0;
		virtual void deleteEvent (int id) = 0;
};


class EventManager {
	private:
		static EventManager *_obj;
		EventManager ();
	
		std::map <std::string, std::shared_ptr <EventType>> _types;
	public:
		static EventManager &getManager ();
	
		void registerEventType (std::shared_ptr <EventType> et);

		std::shared_ptr <EventType> getEventTypeByName (std::string typeName);
		std::shared_ptr <EventType> getEventTypeFromJson (nlohmann::json &data);
		std::shared_ptr <EventType> getEventTypeById (int eventId);

		// returns event id
		int createEvent (nlohmann::json &data, int byUser);
		nlohmann::json getEvent (int eventId);
		void updateEvent (nlohmann::json &data, int byUser);
		void deleteEvent (int eventId, int byUser);

		// Data fields to create different event types
		nlohmann::json getAvailableEventDescriptors ();
};


class BandFoundationEventType: public EventType {
	private:
		struct Data {
			std::string date;
			ParticipantEntity band;
			std::string description;
		};
		
		friend void from_json (const nlohmann::json &j, BandFoundationEventType::Data &d);
		friend void to_json (nlohmann::json &j, const BandFoundationEventType::Data &d);
	public:
		std::string getTypeName () const override;
		std::vector <InputTypeDescriptor> getInputs () const;
		std::vector <std::string> getApplicableEntityTypes () const;
		
		nlohmann::json getEventDescriptor () override;

		int createEvent (nlohmann::json &data) override;
		nlohmann::json getEvent (int id) override;
		void updateEvent (nlohmann::json &data) override;
		void deleteEvent (int id) override;
};

void from_json (const nlohmann::json &j, BandFoundationEventType::Data &d);
void to_json (nlohmann::json &j, const BandFoundationEventType::Data &d);




template <class T>
T EventType::getParameter (std::string name, nlohmann::json &data) {
	if (data.contains (name) == false) {
		throw UserSideEventException (
			std::string ("Нет поля '") + name + "' в переданной структуре данных" 
		);
	}

	try {
		return data[name].get <T>();
	} catch (nlohmann::json::exception &e) {
		throw UserSideEventException (
			std::string ("Невозможно привести поле '") + name + "' к необходимому типу"
		);
	}
}