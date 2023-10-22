
#pragma once

#include <fstream>
#include <memory>
#include <map>
#include <string>
#include <thread>
#include <mutex>
#include <queue>

#include <pqxx/pqxx>
#include "nlohmann-json/json.hpp"

#include "utils.hpp"


struct DatabaseConnection;
class OwnedConnection;
class DBQueueTicket;
class Database;
void execSqlFile (std::string path);

struct DatabaseConnection {
	std::shared_ptr <pqxx::connection> conn;

	DatabaseConnection (const std::shared_ptr <pqxx::connection>& conn);
	DatabaseConnection (const DatabaseConnection &dbconn);
};


class OwnedConnection {
	private:
		int _connId = -1;
		Database *_db = nullptr;
	public:
		std::shared_ptr <pqxx::connection> conn;

		OwnedConnection (Database *db, int connId, std::shared_ptr <pqxx::connection> conn);
		OwnedConnection (const OwnedConnection& conn) = delete; // Can't copy a connection
		OwnedConnection (OwnedConnection&& conn);
		~OwnedConnection ();

		void release ();

		pqxx::connection *operator->();
};


/**********
 * Class for a DB queue ticket.
 * One `shared_ptr` to it is held by the `Database`'s queue, other is held by the thread waiting for connection.
 * Lock is active once the ticket is returned to the awaiting thread. Once a connection is assigned, the lock is unlocked and the thread can `getConnectionId()`.
*/ 
class DBQueueTicket {
	private:
		int _connId = -1;
	public:
		std::mutex mutex;
		std::unique_lock <std::mutex> lock;

		DBQueueTicket ();
		void assignConnection (int connId);
		int getConnectionId ();
};


/*********
 * Thread-safe database class
 * 
 * Every thread trying to access the database must get an `OwnedConnection` by calling `Database::connect`.
 * `OwnedConnection` objects will release its connection to be taken by other threads automatically on destruction or by calling `.release()`.
 * If no connection is currently available, `Database::connect` will return one as soon as one is freed.
*/
class Database {
	private:
		bool _started = false;

		std::string _host, _port, _user, _password;

		std::vector <DatabaseConnection> _conns;
		
		std::queue <int> _freeCons; // List of indices of currently available connections
		std::mutex _freeConsMutex;

		std::queue <std::shared_ptr <DBQueueTicket> > _awaitingConn; // Queue of tickets waiting for connections
		std::mutex _awaitingConnMutex;

	public:
		Database ();
		void init (std::string configFilePath, int initConnections);
		int readConfigFile (std::string configFilePath);
		std::string generateConnString ();
		std::string getServerVersion ();
		bool started();

		std::shared_ptr <DBQueueTicket> queueForConnection (); // Queue a ticket for a connection
		OwnedConnection connect (); // Get a free connection or queue to recieve one as it is freed
		void freeConnection (int connId); // Assign a connection to a thread that is waiting or add it to the list of free connections if no one needs it at the moment
};

extern Database database;

