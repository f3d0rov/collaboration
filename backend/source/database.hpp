
#pragma once

#include <fstream>

#include <memory>
#include <map>
#include <set>
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
std::string lowercase (std::string s);

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


template <class T> 	std::string wrapType (const T &t, pqxx::work &work);
template <>			std::string wrapType <int> (const int &t, pqxx::work &work);
template <>			std::string wrapType <std::string> (const std::string &s, pqxx::work &work);


// Converts JSON object to a set of `UPDATE ... SET x=fromJson, y=fromJson...;` queries
class UpdateQueryElementInterface {
	public:
		~UpdateQueryElementInterface ();

		virtual std::string getSqlColName () const = 0;
		virtual std::string getSqlTableName () const = 0;
		virtual std::string getJsonElementName () const = 0;
		virtual std::string getWrappedValue (const nlohmann::json &j, pqxx::work &w) = 0;

		virtual bool isPresentIn (const nlohmann::json &j);
		std::string getQuery (const nlohmann::json &value, bool notFirst, pqxx::work &w);
};

typedef std::shared_ptr <UpdateQueryElementInterface> UQEI_ptr;


template <class T>
class UpdateQueryElement: public UpdateQueryElementInterface {
	private:
		std::string _jsonElementName;
		std::string _sqlColName;
		std::string _sqlTableName;

	public:
		UpdateQueryElement (std::string jsonElementName, std::string sqlColName, std::string sqlTableName);
		static UQEI_ptr make (std::string jsonElementName, std::string sqlColName, std::string sqlTableName);

		std::string getSqlColName () const override;
		std::string getSqlTableName () const override;
		std::string getJsonElementName () const override;
		virtual std::string getWrappedValue (const nlohmann::json &value, pqxx::work &w) override;
};

class UpdateQueryGenerator {
	public:
		struct TableColumnValue {
			std::string table, col, value;

			TableColumnValue (std::string table, std::string col, std::string value);
			std::string getWhereClause ();
		};

	private:
		class SingleTableQuery {
			private:
				std::string _query;
				UpdateQueryGenerator::TableColumnValue _key;
				bool _updated = false;
			public:
				SingleTableQuery (std::string tableName, UpdateQueryGenerator::TableColumnValue key);
				void addQuery (std::string query);
				bool empty ();
				std::string finalized ();
		};

	private:
		std::set <std::string> _presentTables;
		std::map <std::string, UpdateQueryGenerator::TableColumnValue> _tableKeyCols;
		std::vector <UQEI_ptr> _elems;
		

		std::map <std::string, UpdateQueryGenerator::SingleTableQuery> _generateSTQS ();
		void _parseJson (const nlohmann::json &j, std::map <std::string, UpdateQueryGenerator::SingleTableQuery> &stqs, pqxx::work &w);
		std::string _generateFinalQuery (std::map <std::string, UpdateQueryGenerator::SingleTableQuery> &stqs);

	public:
		UpdateQueryGenerator (std::vector <UQEI_ptr> &&elems, const std::vector <UpdateQueryGenerator::TableColumnValue> &keys);
		std::string queryFor (const nlohmann::json input, pqxx::work &w);
};



#include "database.tcc"
