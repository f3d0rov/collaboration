
#pragma once

#include "events.hpp"


class SingleEntityRelatedEventType;
class BandFoundationEventType;
class BandJoinEventType;
class BandLeaveEventType;

class SinglePublicationEventType;


class SingleEntityRelatedEventType: virtual public EventType {
	protected:
		struct Data {
			std::string date;
			ParticipantEntity entity;
			std::string description;
		};

		friend void from_json (const nlohmann::json &j, SingleEntityRelatedEventType::Data &d);
		friend void to_json (nlohmann::json &j, const SingleEntityRelatedEventType::Data &d);

	public:
		virtual std::vector <InputTypeDescriptor> getInputs () const override;

		virtual int createEvent (nlohmann::json &data) override;
		virtual nlohmann::json getEvent (int id) override;
		virtual int updateEvent (nlohmann::json &data) override;
		// Default delete

		virtual std::string getRelatedEntityType () const = 0;
		virtual std::string getRelatedEntityPromptString () const = 0;
		virtual std::string getDatePrompt () const = 0;

		int getRelatedEntityForEvent (OwnedConnection &work, int eventId);
};

void from_json (const nlohmann::json &j, SingleEntityRelatedEventType::Data &d);
void to_json (nlohmann::json &j, const SingleEntityRelatedEventType::Data &d);


class BandFoundationEventType: virtual public PersonOnlyEventType, virtual public SingleEntityRelatedEventType {
	public:
		std::string getTypeName () const override;
		std::string getDisplayName () const override;
		std::string getTitleFormat () const override;

		virtual std::string getRelatedEntityType () const override;
		virtual std::string getRelatedEntityPromptString () const override;
		virtual std::string getDatePrompt () const override;
};


class BandJoinEventType: public SingleEntityRelatedEventType, public PersonOnlyEventType {
	public:
		std::string getTypeName () const override;
		std::string getDisplayName () const override;
		std::string getTitleFormat () const override;

		virtual std::string getRelatedEntityType () const override;
		virtual std::string getRelatedEntityPromptString () const override;
		virtual std::string getDatePrompt () const override;
};


class BandLeaveEventType: public SingleEntityRelatedEventType, public PersonOnlyEventType {
	public:
		std::string getTypeName () const override;
		std::string getDisplayName () const override;
		std::string getTitleFormat () const override;

		virtual std::string getRelatedEntityType () const override;
		virtual std::string getRelatedEntityPromptString () const override;
		virtual std::string getDatePrompt () const override;
};


class SinglePublicationEventType: public AllEntitiesEventType {
	private:
		struct Data {
			ParticipantEntity author;
			std::string song;
			std::string description;
			std::string date;
		};

		friend void from_json (const nlohmann::json &j, SinglePublicationEventType::Data &d);
		friend void to_json (nlohmann::json &j, const SinglePublicationEventType::Data &d);

	public:
		std::string getTypeName () const override;
		std::string getDisplayName () const override;
		std::string getTitleFormat () const override;

		std::vector <InputTypeDescriptor> getInputs () const override;
		
		int createEvent (nlohmann::json &data) override;
		nlohmann::json getEvent (int id) override;
		int updateEvent (nlohmann::json &data) override;

		int getAuthorForEvent (OwnedConnection &work, int eventId);

		static int getEventIdForSong (int songId);

		void indexSingle (int eventId, int albumId);
};

void from_json (const nlohmann::json &j, SinglePublicationEventType::Data &d);
void to_json (nlohmann::json &j, const SinglePublicationEventType::Data &d);

