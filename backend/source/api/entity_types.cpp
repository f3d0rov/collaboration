
#include "entity_types.hpp"


std::string PersonEntityType::getTitleString () const {
	return "Имя";
}

std::string PersonEntityType::getStartDateString () const {
	return "Дата рождения";
}

std::string PersonEntityType::getEndDateString () const {
	return "Дата смерти";
}

bool PersonEntityType::hasEndDate () const {
	return true;
}

bool PersonEntityType::squareImage () const {
	return false;
}

std::string PersonEntityType::getTypeName () const {
	return "Личность";
}

std::string PersonEntityType::getTypeId () const {
	return "person";
}




std::string BandEntityType::getTitleString () const {
	return "Имя";
}

std::string BandEntityType::getStartDateString () const {
	return "Дата основания";
}

bool BandEntityType::squareImage () const {
	return true;
}

std::string BandEntityType::getTypeName () const {
	return "Группа";
}

std::string BandEntityType::getTypeId () const {
	return "band";
}
