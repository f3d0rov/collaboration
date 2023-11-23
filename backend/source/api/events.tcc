#pragma once

#include "events.hpp"


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

