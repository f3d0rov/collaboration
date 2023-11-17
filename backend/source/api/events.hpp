
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
};

void to_json (nlohmann::json &j, const InputTypeDescriptor &pe);


class EventType {
	private:
		template <class T> 	std::string wrapType (const T &t, pqxx::work &work);
		template <>			std::string wrapType <int> (const int &t, pqxx::work &work);
		template <>			std::string wrapType <std::string> (const std::string &s, pqxx::work &work);
	protected:
		template <class T> T getParameter (std::string name, nlohmann::json &data);
		template <class T> std::string getUpdateQueryString (std::string param, nlohmann::json &data, pqxx::work &work, bool &notFirst);
		std::vector <ParticipantEntity> getParticipants (nlohmann::json &data);
		
	public:
		virtual std::string getTypeName () const = 0;
		virtual std::string getDisplayName () const = 0;
		virtual std::string getTitleFormat () const = 0;
		
		virtual std::vector <InputTypeDescriptor> getInputs () const = 0;
		virtual std::vector <std::string> getApplicableEntityTypes () const = 0;

		virtual nlohmann::json getEventDescriptor ();

		virtual int createEvent (nlohmann::json &data) = 0;
		virtual nlohmann::json getEvent (int id) = 0;
		virtual void updateEvent (nlohmann::json &data) = 0;
		virtual void deleteEvent (int id); // Default implementation deletes corresponding row in events table
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

template <class T> std::string EventType::wrapType (const T &t, pqxx::work &work) {
	throw std::logic_error ("std::string EventType::wrapType<T> вызван с непредусмотренным типом данных");
}


template <class T>
std::string EventType::getUpdateQueryString (std::string param, nlohmann::json &data, pqxx::work &work, bool &notFirst) {
	if (data.contains (param)) {
		T value = this->getParameter<T> (param, data);
		std::string result = (notFirst ? "," : "") + param + "=";
		notFirst = true;
		result += this->wrapType<T> (value, work);
		return result;
	}
	return "";
}
