
#include "database.hpp"


DatabaseConnection::DatabaseConnection (const std::shared_ptr <pqxx::connection>& conn): conn (conn) {

}

DatabaseConnection::DatabaseConnection (const DatabaseConnection &dbconn):
conn (dbconn.conn) {

}

OwnedConnection::OwnedConnection (Database *db, int connId, std::shared_ptr <pqxx::connection> conn):
_db (db), _connId (connId), conn (conn) {
	
}

OwnedConnection::OwnedConnection (OwnedConnection&& conn) {
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
