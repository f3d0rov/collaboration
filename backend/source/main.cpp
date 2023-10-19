
#include <iostream>
#include <thread>

#include <pqxx/pqxx>
#include "civetweb/CivetServer.h"

#include "args.hpp"
#include "utils.hpp"

#include "resource.hpp"
#include "web_resource.hpp"

#define DEFAULT_PORT "8080"
#define DEFAULT_REQUEST_TIMEOUT_TIME_MS "10000"
#define DEFAULT_CIVETWEB_ERROR_LOG_FILE "civetweb-errors.log"

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
				ArgOption ("", "dbconfig", "Путь к файлу .json с данными для подключения к PostgreSQL", true)
				/*,
				ArgOption ("", "db-connections", "Number of initial connections to PostgreSQL", true)*/
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
	
	mg_context* ctx = startCivetweb (argParser);
	if (ctx == nullptr) {
		logger << "Не получилось запустить CivetWeb" << std::endl;
		return -1;
	}

	std::string frontendDir = argParser.getArgValue ("index", DEFAULT_INDEX_DIRECTORY_PATH);
	if (!argParser.hasArg ("index")) chdirToExecutableDirectory (argv[0]);

	SharedDirectory sharedFiles (ctx, frontendDir, false);

	while (1) { // Ждем входящие подключения
		std::this_thread::sleep_for (std::chrono::seconds (1));
	}

	mg_stop (ctx); // Останавливаем сервер
	mg_exit_library ();

	logger << "Сервер COLLABORATION. остановлен" << std::endl;

	return 0;
}

