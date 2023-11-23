#pragma once

#include "database.hpp"


template <class T>
std::string wrapType (const T &t, pqxx::work &work) {
	throw std::logic_error ("std::string EventType::wrapType<T> вызван с непредусмотренным типом данных");
}

template <class T>
UpdateQueryElement<T>::UpdateQueryElement (std::string jsonElementName, std::string sqlColName, std::string sqlTableName):
_jsonElementName (jsonElementName), _sqlColName (sqlColName), _sqlTableName (sqlTableName) {

}

template <class T>
UQEI_ptr UpdateQueryElement<T>::make (std::string jsonElementName, std::string sqlColName, std::string sqlTableName) {
	return std::make_shared <UpdateQueryElement<T>> (jsonElementName, sqlColName, sqlTableName);
}


template <class T>
std::string UpdateQueryElement<T>::getSqlColName () const {
	return this->_sqlColName;
}

template <class T>
std::string UpdateQueryElement<T>::getSqlTableName () const {
	return this->_sqlTableName;
}

template <class T>
std::string UpdateQueryElement<T>::getJsonElementName () const {
	return this->_jsonElementName;
}

template <class T>
std::string UpdateQueryElement<T>::getWrappedValue (const nlohmann::json &value, pqxx::work &w) {
	return wrapType <T> (value.get<T>(), w);
}