
#include <iostream>
#include <thread>

#include <pqxx/pqxx>
#include "civetweb/CivetServer.h"

#include "args.hpp"
#include "utils.hpp"

#include "mailer.hpp"

#include "database.hpp"
#include "resource.hpp"
#include "web_resource.hpp"
#include "api_resource.hpp"

#include "api/randomSearchPromptResource.hpp"
#include "api/user_auth.hpp"

#define DEFAULT_PORT "8080"
#define DEFAULT_REQUEST_TIMEOUT_TIME_MS "10000"
#define DEFAULT_CIVETWEB_ERROR_LOG_FILE "civetweb-errors.log"

#define DEFAULT_DB_CONFIG_FILE "db-config.json"
#define DEFAULT_DB_CONNECTION_COUNT "4"

#define DEFAULT_INDEX_DIRECTORY_PATH "../../frontend/"


int logCivetwebMessage (const mg_connection *conn, const char *message) {
	logger << "CivetWeb message: " << message << std::endl;
	return 1;
}

mg_context *startCivetweb (ArgsParser &argParser) {
	std::string port = argParser.getArgValue ("port", DEFAULT_PORT);
	std::string requestTimeout = argParser.getArgValue ("request-timeout", DEFAULT_REQUEST_TIMEOUT_TIME_MS);
	std::string civetwebErrorLogFile = argParser.getArgValue ("civetweb-error-log", DEFAULT_CIVETWEB_ERROR_LOG_FILE);

	const char* civetwebOptions[] = {
		"listening_ports", port.c_str(),
		"request_timeout_ms", requestTimeout.c_str(),
		"error_log_file", civetwebErrorLogFile.c_str(),
		0
	};

	logger << "Запускаем CivetWeb на порте " << port << ", таймаут запроса: " << requestTimeout << " мс" << std::endl;

	mg_callbacks callbacks;
	memset(&callbacks, 0, sizeof (callbacks)); // Clear the created object
	callbacks.log_message = logCivetwebMessage;

	mg_context *ctx = nullptr;
	int err = 0;

	mg_init_library (0); // Initialize Civetweb
	ctx = mg_start (&callbacks, 0, civetwebOptions); // Start the webserver with callbacks and options
	
	return ctx;
}

void resetDatabase () {
	execSqlFile ("sql/reset-db.sql");
}

void setupDatabase () {
	execSqlFile ("sql/make-db.sql");
}


int main (int argc, const char *argv[]) {
	logger.init();
	logger << std::endl;
	ArgsParser argParser;
	
	try {
		argParser.setupOptions (
			{
				ArgOption ("v", "version", "Вывод версии программы"),
				ArgOption ("h", "help", "Вывод доступных опций"),
				ArgOption ("i", "index", "Путь к директории с файлами фронтенда", true),
				ArgOption ("p", "port", "Порт для входящих запросов", true),
				ArgOption ("", "request-timeout", "Таймаут запросов", true),
				ArgOption ("", "civetweb-error-log", "Файл для записи ошибок Civetweb", true),
				ArgOption ("", "db-config", "Путь к файлу .json с данными для подключения к PostgreSQL", true),
				ArgOption ("", "db-connections", "Количество одновременных соединений с PostgreSQL", true),
				ArgOption ("", "remake-db", "Удалить и заново создать базу данных"),
				ArgOption ("", "smtp-config", "Путь к файлу .json с данными для подключения к SMTP-серверу", true)
			}
		);

		if (argParser.parseArgs (argc, argv)) {
			argParser.printHelp();
			return 0;
		}

	} catch (std::exception e) {
		std::cout << "Не получилось обработать аргументы: " << e.what() << std::endl;
		argParser.printHelp();
		return -1;
	}

	if (argParser.hasArg ("help")) {
		argParser.printHelp();
		return 0;
	}

	if (argParser.hasArg ("version")) {
		std::cout << "COLLABORATION." << std::endl;
		std::cout << "Автор: f3d0rov [evnfdrv@gmail.com]" << std::endl;
		std::cout << "Версия CivetWeb: " << mg_version() << std::endl;
		return 0;
	}

	logger << "Запускаем сервер COLLABORATION. " << std::endl;

	std::string frontendDir = argParser.getArgValue ("index", DEFAULT_INDEX_DIRECTORY_PATH);
	if (!argParser.hasArg ("index")) chdirToExecutableDirectory (argv[0]);
	
	database.init (
		argParser.getArgValue ("db-config", DEFAULT_DB_CONFIG_FILE),
		std::stoi(argParser.getArgValue ("db-connections", DEFAULT_DB_CONNECTION_COUNT))
	);
	if (!database.started()) {
		logger << "Не удалось подключиться к базе данных" << std::endl;
		return -1;
	}

	if (argParser.hasArg ("remake-db")) {
		logger << "Пересоздаем базу данных..." << std::endl;
		resetDatabase ();
		setupDatabase ();
	}
	try {
		mailer.init (argParser.getArgValue ("smtp-config", DEFAULT_SMTP_CONFIG_FILE));
	} catch (std::exception &e) {
		logger << "Не удалось подключиться к серверу SMTP: " << e.what() << std::endl;
		return -1;
	}

	mg_context* ctx = startCivetweb (argParser);
	if (ctx == nullptr) {
		logger << "Не получилось запустить CivetWeb" << std::endl;
		return -1;
	}


	SharedDirectory sharedFiles (ctx, frontendDir, true);
	WebResource indexPage (ctx, "", frontendDir + "/index.html");
	WebResource personPage (ctx, "p", frontendDir + "/person.html");

	Resource api404 (ctx, "api");
	RandomSearchPromptResource RandomSearchPromptResource (ctx, "api/rsp");

	UserLoginResource userLoginResource 				(ctx, "api/u/login");
	UserLogoutResource userLogoutResource 				(ctx, "api/u/logout");
	UserRegisterResource userRegisterResource 			(ctx, "api/u/register");
	CheckUsernameAvailability checkUsernameAvailability	(ctx, "api/u/check_username");
	CheckEmailAvailability checkEmailAvailability 		(ctx, "api/u/check_email");
	CheckSessionResource checkSessionResource 			(ctx, "api/u/whoami");

	while (1) { // Ждем входящие подключения
		std::this_thread::sleep_for (std::chrono::seconds (1));
	}

	mg_stop (ctx); // Останавливаем сервер
	mg_exit_library ();

	logger << "Сервер COLLABORATION. остановлен" << std::endl;

	return 0;
}

