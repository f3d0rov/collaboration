
#pragma once

#include "entity_manager.hpp"


class PersonEntityType: public EntityTypeInterface {
	public:
		std::string getTitleString () const final;
		std::string getStartDateString () const final;
		std::string getEndDateString () const final;

		bool hasEndDate () const final;
		bool squareImage () const final;

		std::string getTypeName () const final;
		std::string getTypeId () const final;
};

class BandEntityType: public EntityTypeInterface {
	public:
		std::string getTitleString () const final;
		std::string getStartDateString () const final;

		bool squareImage () const final;

		std::string getTypeName () const final;
		std::string getTypeId () const final;
};
