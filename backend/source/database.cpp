
#include "database.hpp"


DatabaseConnection::DatabaseConnection (const std::shared_ptr <pqxx::connection>& conn): conn (conn) {

}

DatabaseConnection::DatabaseConnection (const DatabaseConnection &dbconn):
conn (dbconn.conn) {

}

OwnedConnection::OwnedConnection (Database *db, int connId, std::shared_ptr <pqxx::connection> conn):
_connId (connId), _db (db), conn (conn) {
	work = std::make_unique <pqxx::work> (*conn);
}

OwnedConnection::OwnedConnection (OwnedConnection&& conn):
work (std::move (conn.work)) {
	std::swap (this->_connId, conn._connId);
	std::swap (this->_db, conn._db);
	std::swap (this->conn, conn.conn);
}

OwnedConnection::~OwnedConnection () {
	this->release();
}

void OwnedConnection::release () {
	if (this->_connId != -1) {
		this->_db->freeConnection (this->_connId);
		this->_db = nullptr;
		this->_connId = -1;
		this->conn = nullptr;
	}
}

pqxx::connection *OwnedConnection::operator->() {
	return this->conn.get();
}
void OwnedConnection::logQuery (std::string_view query) {
	if (common.logSql) logger << "SQL QUERY [" << this->_connId << "]: " << query << std::endl;
}

int OwnedConnection::id () {
	return this->_connId;
}

pqxx::result OwnedConnection::exec (std::string query) {
	this->logQuery (query);
	return this->work->exec (query);
}

pqxx::result OwnedConnection::exec0 (std::string query) {
	this->logQuery (query);
	return this->work->exec0 (query);
}

pqxx::row OwnedConnection::exec1 (std::string query) {
	this->logQuery (query);
	return this->work->exec1 (query);
}

std::string OwnedConnection::quote (std::string raw) {
	return this->work->quote (escapeHTML(raw));
}

std::string OwnedConnection::quoteDontEscapeHtml (std::string raw) {
	return this->work->quote (raw);
}

void OwnedConnection::commit () {
	this->work->commit();
	if (common.logSql) logger << "SQL COMMIT [" << this->_connId << "]" << std::endl;
}


DBQueueTicket::DBQueueTicket () {
	this->lock = std::unique_lock (this->mutex);
}

void DBQueueTicket::assignConnection (int connId) {
	this->_connId = connId;
	this->lock.unlock ();
}

int DBQueueTicket::getConnectionId () {
	return this->_connId;
}

Database::Database () {

}

void Database::init (std::string configFilePath, int initConnections) {
	if (this->readConfigFile (configFilePath)) {
		return;
	}

	try {
		for (int i = 0; i < initConnections; i++) {
			this->_conns.push_back ({std::make_shared <pqxx::connection> (this->generateConnString())});
			this->_freeCons.push (i);
		}
	} catch (std::exception &e) {
		logger << "Failed to connect to database: " << e.what() << std::endl;
		return;
	}
	
	auto conn0 = this->_conns[0].conn;
	logger << "Connected to database '" << conn0->dbname() << "' as user '" << conn0->username() << "'" << std::endl;
	logger << "Database at host '" << conn0->hostname() << ":" << conn0->port() << 
				"', PostgreSQL v." << this->getServerVersion() << ", protocol v." << conn0->protocol_version() << std::endl;
	logger << initConnections << " connections active" << std::endl;

	this->_started = true;
}

int Database::readConfigFile (std::string configFilePath) {
	logger << "Reading database config from '" << configFilePath << "'" << std::endl;
	
	std::ifstream f(configFilePath);
	if (!f.is_open()) {
		logger << "Failed to open database config file '" << configFilePath << "'" << std::endl;
		return -1;
	}

	nlohmann::json json;
	try {
		json = nlohmann::json::parse (f);
	} catch (nlohmann::json::exception &e) {
		logger << "Failed to parse database configuration file '" << configFilePath << "': " << e.what() << std::endl;
		return -1;
	}

	try {
		this->_host = json["host"].template get<std::string>();
		this->_port = json["port"].template get<std::string>();
		this->_user = json["user"].template get<std::string>();
		this->_password = json["password"].template get<std::string>();
	} catch (nlohmann::json::exception &e) {
		logger << "Failed to read database configuration file '" << configFilePath << "': " << e.what() << std::endl;
		return -1;
	}

	return 0;
}

std::string Database::generateConnString () {
	return std::string ("host=") + this->_host + " port=" + this->_port + " user=" + this->_user + " password=" + this->_password;
}

std::string Database::getServerVersion () {
	int version = this->_conns[0].conn->server_version();
	return std::to_string (version / 10000) + "." + std::to_string (version % 10000 / 100) + "." + std::to_string (version % 100);
}

std::shared_ptr <DBQueueTicket> Database::queueForConnection () {
	std::unique_lock awaitingConnLock(this->_awaitingConnMutex); 
	auto newTicket = std::make_shared <DBQueueTicket> ();
	_awaitingConn.push (newTicket); // One `shared_ptr` goes to `_awaitingConn` queue where it is picked up by threads calling `Database::freeConnection`
	return newTicket;	// Second goes to the thread that is requesting the connection to lock it until one becomes available
}

OwnedConnection Database::connect () {
	std::unique_lock freeConsLock (this->_freeConsMutex); // Ensure `_freeCons` isn't modified by other threads

	if (!this->_freeCons.empty()) { // If there are unassigned connections we will take the first one
		int connId = this->_freeCons.front();
		this->_freeCons.pop();
		return OwnedConnection (this, connId, this->_conns[connId].conn); // Return the free connection
	} else {
		std::shared_ptr <DBQueueTicket> queueTicket = this->queueForConnection(); 	// Queue for connection. `queueTicket->mutex` is initially locked;
																					// it will be unlocked once we are allocated a connection.
		freeConsLock.unlock(); 	// Don't care about the free connections anymore, release the lock so other threads trying to connect to DB could see it's empty
								// Unlock it only after queueing for connection so that Database::freeConnection wouldn't be able to miss the ticket
		std::unique_lock waitForQueue (queueTicket->mutex); // TWait for mutex ownership (will recieve it once a connection is allocated to this ticket)
		int connId = queueTicket->getConnectionId (); // Get id for the allocated connection
		return OwnedConnection (this, connId, this->_conns[connId].conn); // Return the allocated connection
	}
}

void Database::freeConnection (int connId) {
	std::unique_lock freeConsLock (this->_freeConsMutex);
	std::unique_lock awaitingConnLock(this->_awaitingConnMutex);

	if (!_awaitingConn.empty()) { // If there are threads that are waiting for a connection to be allocated
		auto ticket = _awaitingConn.front(); // Get first ticket
		_awaitingConn.pop(); 
		ticket->assignConnection (connId); // Assign freed connection to the ticket
	} else {
		_freeCons.push (connId);
	}
}

bool Database::started () {
	return this->_started;
}

Database database;

void execSqlFile (std::string path) {
	std::string file = readFile (path);
	auto conn = database.connect ();
	
	try {
		conn.exec (file);
		conn.commit ();
	} catch (std::exception &e) {
		logger << "Ошибка при исполнении файла '" + path + "': " + e.what() << std::endl;
		throw;
	}
}


template <>
std::string wrapType <int> (const int &t, OwnedConnection &work) {
	return std::to_string (t);
}

template <>
std::string wrapType <std::string> (const std::string &s, OwnedConnection &work) {
	return work.quote (s);
}




UpdateQueryElementInterface::~UpdateQueryElementInterface () = default;

bool UpdateQueryElementInterface::isPresentIn (const nlohmann::json &j) {
	return j.contains (this->getJsonElementName());
}

std::string UpdateQueryElementInterface::getQuery (const nlohmann::json &j, bool notFirst, OwnedConnection &w) {
	std::string comma = notFirst ? "," : "";
	return comma + this->getSqlColName() + "=" + this->getWrappedValue (j.at (this->getJsonElementName()), w);
}




UpdateQueryGenerator::TableColumnValue::TableColumnValue (std::string table, std::string col, std::string value):
table (table), col (col), value (value) {

}

std::string UpdateQueryGenerator::TableColumnValue::getWhereClause () {
	return std::string (" WHERE ") + this->col + "=" + this->value;
}



UpdateQueryGenerator::SingleTableQuery::SingleTableQuery (std::string tableName, UpdateQueryGenerator::TableColumnValue key):
_key (key) {
	this->_query = std::string{};
	this->_query = "UPDATE " + tableName + " SET ";
}

void UpdateQueryGenerator::SingleTableQuery::addQuery (std::string query) {
	this->_updated = true;
	this->_query += query;
}

bool UpdateQueryGenerator::SingleTableQuery::empty () {
	return !this->_updated;
}

std::string UpdateQueryGenerator::SingleTableQuery::finalized () {
	return this->_query + this->_key.getWhereClause () + ";";
}



UpdateQueryGenerator::UpdateQueryGenerator (std::vector <UQEI_ptr> &&elems, const std::vector <UpdateQueryGenerator::TableColumnValue> &keys):
_elems (elems) {
	for (const auto &i: keys) {
		this->_tableKeyCols.insert ({i.table, i});
	}

	for (const auto &i: this->_elems) {
		if (this->_tableKeyCols.contains (i->getSqlTableName ()) == false) {
			throw std::logic_error (
				std::string ("UpdateQueryGenerator: не найден ключ для таблицы '") + i->getSqlTableName() + "'"
			);
		}
		this->_presentTables.emplace (i->getSqlTableName());
	}
}

std::map <std::string, UpdateQueryGenerator::SingleTableQuery> UpdateQueryGenerator::_generateSTQS () {
	std::map <std::string, UpdateQueryGenerator::SingleTableQuery> stqs;
	for (auto &i: this->_presentTables) {
		stqs.insert ({i, UpdateQueryGenerator::SingleTableQuery (i, this->_tableKeyCols.at (i))});
	}
	return stqs;
}

void UpdateQueryGenerator::_parseJson (const nlohmann::json &j, std::map <std::string, UpdateQueryGenerator::SingleTableQuery> &stqs, OwnedConnection &w) {
	for (auto &elem: this->_elems) {
		if (elem->isPresentIn (j)) {
			UpdateQueryGenerator::SingleTableQuery &stq = stqs.at (elem->getSqlTableName());
			stq.addQuery ( elem->getQuery (j, !stq.empty(), w) );
		}
	}
}

std::string UpdateQueryGenerator::_generateFinalQuery (std::map <std::string, UpdateQueryGenerator::SingleTableQuery> &stqs) {
	std::string finalQuery = "";

	for (auto &i: stqs) {
		UpdateQueryGenerator::SingleTableQuery &stq = i.second;
		if (stq.empty()) continue;
		finalQuery += stq.finalized();
	}

	return finalQuery;
}

std::string UpdateQueryGenerator::queryFor (const nlohmann::json input, OwnedConnection &w) {
	std::map <std::string, UpdateQueryGenerator::SingleTableQuery> stqs (this->_generateSTQS ());
	this->_parseJson (input, stqs, w);
	return this->_generateFinalQuery (stqs);
}

