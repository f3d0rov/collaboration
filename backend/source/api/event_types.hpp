
#pragma once

#include "events.hpp"


class SingleEntityRelatedEventType: public EventType {
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
		virtual void updateEvent (nlohmann::json &data) override;
		// Default delete

		virtual std::string getRelatedEntityType () const = 0;
		virtual std::string getRelatedEntityPromptString () const = 0;
		virtual std::string getDatePrompt () const = 0;
};

void from_json (const nlohmann::json &j, SingleEntityRelatedEventType::Data &d);
void to_json (nlohmann::json &j, const SingleEntityRelatedEventType::Data &d);


class BandFoundationEventType: public SingleEntityRelatedEventType {
	public:
		std::string getTypeName () const override;
		std::string getDisplayName () const override;
		std::string getTitleFormat () const override;

		std::vector <std::string> getApplicableEntityTypes () const override;

		virtual std::string getRelatedEntityType () const override;
		virtual std::string getRelatedEntityPromptString () const override;
		virtual std::string getDatePrompt () const override;
};


class BandJoinEventType: public SingleEntityRelatedEventType {
	public:
		std::string getTypeName () const override;
		std::string getDisplayName () const override;
		std::string getTitleFormat () const override;

		std::vector <std::string> getApplicableEntityTypes () const override;

		virtual std::string getRelatedEntityType () const override;
		virtual std::string getRelatedEntityPromptString () const override;
		virtual std::string getDatePrompt () const override;
};


class BandLeaveEventType: public SingleEntityRelatedEventType {
	public:
		std::string getTypeName () const override;
		std::string getDisplayName () const override;
		std::string getTitleFormat () const override;

		std::vector <std::string> getApplicableEntityTypes () const override;

		virtual std::string getRelatedEntityType () const override;
		virtual std::string getRelatedEntityPromptString () const override;
		virtual std::string getDatePrompt () const override;
};
