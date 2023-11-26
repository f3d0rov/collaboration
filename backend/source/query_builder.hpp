#pragma once

#include <string>
#include <functional>
#include "database.hpp"


class _QueryColumn {
	protected:
		std::string _table;
		std::string _column;
		std::string _alias;

		std::weak_ptr <pqxx::result> _result;

	public:
		_QueryColumn (std::string table, std::string column, std::string alias = "");
		~_QueryColumn ();

		virtual std::string query ();
		void setResult (std::weak_ptr<pqxx::result> result);
		pqxx::row::reference getRef (int index);

		std::string table () const;
		std::string column () const;
		std::string alias () const;

		std::string tableCol () const;
};

template <class T>
class QueryColumn: public _QueryColumn {
	public:
		QueryColumn (std::string table, std::string column, std::string alias = "");
		T get (int index);
};


class QueryCondition {
	public:
		virtual ~QueryCondition ();
		virtual std::string condition (OwnedConnection &conn) = 0;
};


typedef std::shared_ptr <QueryCondition> QueryConditionPtr;

namespace Condition {
	template <class A, class B>
	class Equals: public QueryCondition {
		private:
			A _a;
			B _b;
		public:	
			Equals (A a, B b);
			std::string condition (OwnedConnection &conn);
	};

	template <class A, class B>
	QueryConditionPtr equals (A a, B b);
}

template <class T>
std::string wrapForQuery (T a, OwnedConnection &conn);

template <class T>
std::string wrapForQuery (QueryColumn <T> a, OwnedConnection &conn);

template <>
std::string wrapForQuery <const _QueryColumn &> (const _QueryColumn &col, OwnedConnection &conn);


class GetQuery {
	private:
		struct Join {
			std::string table;
			std::string oper;
			QueryConditionPtr on;
		};
	
		std::vector <std::reference_wrapper <_QueryColumn>> _selecting;
		std::string _baseTable;
		std::vector <Join> _joins;
		std::vector <QueryConditionPtr> _wheres;

		std::shared_ptr <pqxx::result> _result;

		std::set<std::string> selectingFromTables ();

	public:
		GetQuery ();
		GetQuery &select (std::vector <std::reference_wrapper <_QueryColumn>> cols);
		GetQuery &from (std::string baseTable);
		GetQuery &where (QueryConditionPtr condition);

		GetQuery &join (std::string table, QueryConditionPtr on);
		GetQuery &loJoin (std::string table, QueryConditionPtr on);
		GetQuery &roJoin (std::string table, QueryConditionPtr on);
		GetQuery &oJoin (std::string table, QueryConditionPtr on);



		std::string makeJoinStatement (OwnedConnection &conn);
		std::string makeWhereStatement (OwnedConnection &conn);
		std::string finalize (OwnedConnection &conn);

		void exec (OwnedConnection &conn);
		void exec ();

		int size();
};


#if 0

class InfoGetter: public GetQuery {
	public:
		QueryColumn <int> id {"table", "id"};
		QueryColumn <int> count {"table", "count"};
		QueryColumn <std::string> name {"table", "name"};
} info;

info.select ({infoGetter.id, infoGetter.count, infoGetter.name}).from ("table").where (Condition::equals (infoGetter.id, 1)).exec (conn);
info.id.get();
info.name.get();


#endif

#include "query_builder.tcc"
