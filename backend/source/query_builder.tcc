
#include "query_builder.hpp"


template <class T>
QueryColumn<T>::QueryColumn (std::string table, std::string column, std::string alias):
_QueryColumn (table, column, alias) {

}

template <class T>
T QueryColumn<T>::get (int index) {
	return this->getRef (index).template as <T>();
}


template <class A, class B>
Condition::Equals<A, B>::Equals (A a, B b):
_a (a), _b (b) {

}

template <class A, class B>
std::string Condition::Equals<A, B>::condition (OwnedConnection &conn) {
	return wrapForQuery (_a, conn) + "=" + wrapForQuery (_b, conn);
}

template <class A, class B>
QueryConditionPtr Condition::equals (A a, B b) {
	return std::make_shared <Condition::Equals<A, B>> (a, b);
}


template <class T>
std::string wrapForQuery (T a, OwnedConnection &conn) {
	return wrapType (a, conn);
}

template <class T>
std::string wrapForQuery (QueryColumn <T> a, OwnedConnection &conn) {
	return wrapForQuery <const _QueryColumn &> (a, conn);
}
