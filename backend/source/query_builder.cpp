
#include "query_builder.hpp"


_QueryColumn::_QueryColumn (std::string table, std::string column, std::string alias):
_table (table), _column (column), _alias (alias) {

}

_QueryColumn::~_QueryColumn () = default;


void _QueryColumn::setResult (std::weak_ptr <pqxx::result> result) {
	this->_result = result;
}

std::string _QueryColumn::query() {
	return this->_table + "." + this->_column + (this->_alias == "" ? "" : (" AS "s + this->_alias));
}


pqxx::row::reference _QueryColumn::getRef (int index) const {
	if (std::shared_ptr<pqxx::result> result = this->_result.lock())
		return result->at (index).at (this->_column);
	throw std::logic_error ("QueryColumn<T>::get: переменная не была получена в запросе или запрос не был выполнен");
}

bool _QueryColumn::isNull (int index) const {
	return this->getRef(index).is_null();
}


std::string _QueryColumn::table () const {
	return this->_table;
}

std::string _QueryColumn::column () const {
	return this->_column;
}

std::string _QueryColumn::alias () const {
	return this->_alias;
}

std::string _QueryColumn::tableCol () const {
	return this->_table + "." + this->_column;
}




QueryCondition::~QueryCondition () = default;



template <>
std::string wrapForQuery <const _QueryColumn &> (const _QueryColumn &col, OwnedConnection &conn) {
	return col.tableCol();
}



GetQuery::GetQuery () {

}

std::set <std::string> GetQuery::selectingFromTables () {
	std::set <std::string> tables;
	for (auto i: this->_selecting) {
		tables.insert (i.get().table());
	}
	return tables;
}


GetQuery &GetQuery::select (std::vector <std::reference_wrapper <_QueryColumn>> cols) {
	this->_selecting.insert (this->_selecting.end(), cols.begin(), cols.end());
	return *this;
}

GetQuery &GetQuery::from (std::string baseTable) {
	this->_baseTable = baseTable;
	return *this;
}

GetQuery &GetQuery::where (QueryConditionPtr condition) {
	this->_wheres.push_back (condition);
	return *this;
}


GetQuery &GetQuery::join (std::string table, QueryConditionPtr on) {
	this->_joins.push_back ({table, "INNER JOIN", on});
	return *this;
}

GetQuery &GetQuery::loJoin (std::string table, QueryConditionPtr on) {
	this->_joins.push_back ({table, "LEFT OUTER JOIN", on});
	return *this;
}

GetQuery &GetQuery::roJoin (std::string table, QueryConditionPtr on) {
	this->_joins.push_back ({table, "RIGHT OUTER JOIN", on});
	return *this;
}

GetQuery &GetQuery::oJoin (std::string table, QueryConditionPtr on) {
	this->_joins.push_back ({table, "OUTER JOIN", on});
	return *this;
}


std::string GetQuery::makeJoinStatement (OwnedConnection &conn) {
	std::string statement = "";
	for (auto i: this->_joins) {
		statement += " "s + i.oper + " " + i.table + " ON " + i.on->condition (conn);
	}
	return statement;
}

std::string GetQuery::makeWhereStatement (OwnedConnection &conn) {
	std::string statement = "WHERE";
	bool notFirst = false;
	for (auto i: this->_wheres) {
		statement += (notFirst ? " AND "s: " "s) + i->condition (conn);
		notFirst = true;
	}
	return statement;
}

std::string GetQuery::finalize (OwnedConnection &conn) {
	std::string query = "SELECT ";
	bool notFirst = false;

	for (auto &i: this->_selecting) {
		query += (notFirst ? ", "s : ""s) + i.get().query();
		notFirst = true;
	}
	
	if (_baseTable == "") throw std::logic_error ("GetQuery::finalize: _baseTable is \"\"");

	query += " FROM "s + this->_baseTable + " ";

	if (this->_joins.size() != 0) query += this->makeJoinStatement (conn);
	if (this->_wheres.size() != 0) query += " "s + this->makeWhereStatement (conn);

	query += ";";
	return query;
}


void GetQuery::exec (OwnedConnection &conn) {
	std::string query = this->finalize (conn);
	pqxx::result result = conn.exec (query);
	this->_result = std::make_shared <pqxx::result> (result);

	for (auto i: this->_selecting) {
		i.get().setResult (this->_result);
	}
}

void GetQuery::exec () {
	auto conn = database.connect ();
	this->exec (conn);
}

int GetQuery::size () {
	return this->_result->size();
}
