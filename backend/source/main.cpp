
#include <iostream>
#include <thread>
#include <locale>

#include <pqxx/pqxx>
#include "civetweb/CivetServer.h"

#include "args.hpp"
#include "utils.hpp"

#include "mailer.hpp"

#include "database.hpp"
#include "resource.hpp"
#include "web_resource.hpp"
#include "api_resource.hpp"

#include "api/random_search_prompt_resource.hpp"
#include "api/search.hpp"
#include "api/user_auth.hpp"
#include "api/page.hpp"

#include "api/events.hpp"
#include "api/event_types.hpp"
#include "api/event_resources.hpp"
#include "api/albums.hpp"

#include "api/song_manager.hpp"


#define DEFAULT_PORT "8080"
#define DEFAULT_HTTPS_PORT "443"
#define DEFAULT_REQUEST_TIMEOUT_TIME_MS "10000"
#define DEFAULT_CIVETWEB_ERROR_LOG_FILE "civetweb-errors.log"

#define DEFAULT_DB_CONFIG_FILE "db-config.json"
#define DEFAULT_DB_CONNECTION_COUNT "4"

#define DEFAULT_INDEX_DIRECTORY_PATH "../../frontend/"


int logCivetwebMessage (const mg_connection *conn, const char *message) {
	logger << "CivetWeb message: " << message << std::endl;
	return 1;
}

int initSSL (void *ssl_ctx, void *user_data) {
	// ssl_ctx_st *ctx = static_cast <ssl_ctx_st *> (ssl_ctx);
	return 0;
}

mg_context *startCivetweb (ArgsParser &argParser) {
	std::setlocale (LC_CTYPE, "ru_RU.UTF-8");
	std::string port = argParser.getArgValue ("port", DEFAULT_PORT);
	std::string securePort = argParser.getArgValue ("secure-port", DEFAULT_HTTPS_PORT);
	std::string requestTimeout = argParser.getArgValue ("request-timeout", DEFAULT_REQUEST_TIMEOUT_TIME_MS);
	std::string civetwebErrorLogFile = argParser.getArgValue ("civetweb-error-log", DEFAULT_CIVETWEB_ERROR_LOG_FILE);

	const char **civetwebOptions = nullptr;

	const char *httpCivetwebOptions[] = {
		"listening_ports", port.c_str(),
		"request_timeout_ms", requestTimeout.c_str(),
		"error_log_file", civetwebErrorLogFile.c_str(),
		0
	};

	std::string ports = port + "," + securePort + "s";

	const char *httpsCivetwebOptions[] = {
		"listening_ports", ports.c_str(),
		"request_timeout_ms", requestTimeout.c_str(),
		"error_log_file", civetwebErrorLogFile.c_str(),
		"ssl_certificate", "./server.pem",
		"ssl_protocol_version", "3",
		"ssl_cipher_list", "DES-CBC3-SHA:AES128-SHA:AES128-GCM-SHA256",
		0
	};

	if (argParser.hasArg ("https")) {
		civetwebOptions = httpsCivetwebOptions;
		common.http = "https";
	} else {
		civetwebOptions = httpCivetwebOptions;
	}
	
	if (!mg_check_feature(2)) {
		logger << "Библиотека Civetweb была собрана без поддержки SSL" << std::endl;
	}

	logger << "Запускаем CivetWeb на порте " << civetwebOptions[1] << ", таймаут запроса: " << requestTimeout << " мс" << std::endl;

	mg_callbacks callbacks;
	memset(&callbacks, 0, sizeof (callbacks)); // Clear the created object
	callbacks.log_message = logCivetwebMessage;
	callbacks.init_ssl = initSSL;

	mg_context *ctx = nullptr;
	// int err = 0;

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


void clearTimedoutPendingEmailConfirmations () {
	auto conn = database.connect();

	std::string removeBadUsersQuery = std::string ()
		+ "DELETE FROM users WHERE uid in "
			+ "(SELECT uid FROM pending_email_confirmation WHERE valid_until < CURRENT_TIMESTAMP);";
	auto result = conn.exec (removeBadUsersQuery);
	if (result.affected_rows() > 0) {
		logger << "Очистил " << result.affected_rows() << " пользователей, не подтвердивших свой адрес почты." << std::endl;
	}
	conn.commit();
}

void occasionalTasks () {
	clearTimedoutPendingEmailConfirmations();
}


int main (int argc, const char *argv[]) {
	_logger.init();
	logger << std::endl;
	ArgsParser argParser;
	
	try {
		argParser.setupOptions (
			{
				ArgOption ("v", "version", "Вывод версии программы"),
				ArgOption ("h", "help", "Вывод доступных опций"),
				ArgOption ("d", "domain", "Адрес сайта", true),
				ArgOption ("s", "https", "Использовать https"),
				ArgOption ("i", "index", "Путь к директории с файлами фронтенда", true),
				ArgOption ("p", "port", "Порт для входящих запросов", true),
				ArgOption ("", "secure-port", "Порт для входящих запросов через HTTPS", true),
				ArgOption ("", "request-timeout", "Таймаут запросов", true),
				ArgOption ("", "civetweb-error-log", "Файл для записи ошибок Civetweb", true),
				ArgOption ("", "db-config", "Путь к файлу .json с данными для подключения к PostgreSQL", true),
				ArgOption ("", "db-connections", "Количество одновременных соединений с PostgreSQL", true),
				ArgOption ("", "remake-db", "Удалить и заново создать базу данных"),
				ArgOption ("", "smtp-config", "Путь к файлу .json с данными для подключения к SMTP-серверу", true),
				ArgOption ("", "no-smtp", "Не отправлять письма"),
				ArgOption ("", "no-cache", "Не кешировать веб-ресурсы"),
				ArgOption ("", "log-sql", "Сохранять в логах все исполненные SQL-запросы"),
				ArgOption ("", "log-width", "Максимальная длина строки в логе (не считая временной отметки)", true)
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

	common.logOverflowLineLength = std::stoi (argParser.getArgValue ("log-width", "120"));

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

	common.domain = argParser.getArgValue ("domain", std::string("localhost:") + argParser.getArgValue ("port", DEFAULT_PORT));

	std::string frontendDir = argParser.getArgValue ("index", DEFAULT_INDEX_DIRECTORY_PATH);
	common.frontendDir = frontendDir;
	common.logSql = argParser.hasArg ("log-sql");
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

	if (!argParser.hasArg ("no-smtp")) {
		try {
			mailer.init (argParser.getArgValue ("smtp-config", DEFAULT_SMTP_CONFIG_FILE));
		} catch (std::exception &e) {
			logger << "Не удалось подключиться к серверу SMTP: " << e.what() << std::endl;
			return -1;
		}
	}
	

	mg_context* ctx = startCivetweb (argParser);
	if (ctx == nullptr) {
		logger << "Не получилось запустить CivetWeb" << std::endl;
		return -1;
	}

	bool dontCache = argParser.hasArg ("no-cache");
	if (dontCache) {
		logger << "Кэширование файлов и ресурсов отключено." << std::endl;
	}

	auto &uploadResourceManager = UploadedResourcesManager::get();
	std::filesystem::path userdataFolder = "./user/";
	uploadResourceManager.setDirectory (userdataFolder);

	EventManager &eventManager = EventManager::getManager();

	eventManager.registerEventType (std::make_shared <BandFoundationEventType>());
	eventManager.registerEventType (std::make_shared <BandJoinEventType>());
	eventManager.registerEventType (std::make_shared <BandLeaveEventType>());
	eventManager.registerEventType (std::make_shared <SinglePublicationEventType>());
	eventManager.registerEventType (std::make_shared <AlbumEventType>());
	logger << "Зарегистрированно типов событий: " << eventManager.size() << std::endl;

	SharedDirectory sharedFiles (ctx, frontendDir, true, dontCache);
	WebResource indexPage (ctx, "", frontendDir + "/index.html", dontCache);
	WebResource personPage (ctx, "e", frontendDir + "/entity.html", dontCache);
	WebResource searchPage (ctx, "search", frontendDir + "/search.html", dontCache);
	WebResource createPage (ctx, "create", frontendDir + "/create_page.html", dontCache);
	WebResource albumPage (ctx, "a", frontendDir + "/album.html", dontCache);

	Resource api404 (ctx, "api");

	RandomSearchPromptResource randomSearchPromptResource (ctx, "api/rsp");
	SearchResource searchResource 						(ctx, "api/search");
	
	TypedSearchResource bandSearchResource				(ctx, "api/search/b", "band");
	TypedSearchResource personSearchResource			(ctx, "api/search/p", "person");
	EntitySearchResource entitySearchResource			(ctx, "api/search/entities");

	UserLoginResource userLoginResource 				(ctx, "api/u/login");
	UserLogoutResource userLogoutResource 				(ctx, "api/u/logout");
	UserRegisterResource userRegisterResource 			(ctx, "api/u/register");
	ConfirmRegistrationResource confirmRegistrationResource (ctx, "confirm");
	CheckUsernameAvailability checkUsernameAvailability	(ctx, "api/u/check_username");
	CheckEmailAvailability checkEmailAvailability 		(ctx, "api/u/check_email");
	CheckSessionResource checkSessionResource 			(ctx, "api/u/whoami");

	CreatePageResource createPageApiResource 			(ctx, "api/create");
	UploadResource uploadResource 						(ctx, "api/upload");
	uploadResourceManager.setUploadUrl (std::string(uploadResource.uri()));
	RequestPictureChangeResource requestPicChange 		(ctx, "api/askchangepic", std::string(uploadResource.uri()));

	DynamicDirectory userPics 							(ctx, "imgs", "./user/");
	uploadResourceManager.setDownloadUri (std::string(userPics.uri()));

	EntityDataResource EntityDataResource 				(ctx, "api/p", std::string(uploadResource.uri()));

	GetEntityEventDescriptorsResource eventDescriptorsResource (ctx, "api/events/types");
	CreateEntityEventResource createEntityEventResource	(ctx, "api/events/create");
	GetEntityEventsResource getEntityEventsResource		(ctx, "api/events/getfor");
	UpdateEntityEventResource updateEntityEventResource (ctx, "api/events/update");
	DeleteEntityEventResource deleteEntityEventResource (ctx, "api/events/delete");
	EventReportTypesResource eventReportTypesResource 	(ctx, "api/events/report_types");
	ReportEntityEventResource reportEventResource 		(ctx, "api/events/report");

	AlbumDataResource albumDataResource 				(ctx, "api/albums/get");
	RequestAlbumImageChangeResource albumImageUploader	(ctx, "api/album/askchangepic");

	while (1) { // Ждем входящие подключения
		occasionalTasks ();	
		std::this_thread::sleep_for (std::chrono::minutes (10));
	}

	mg_stop (ctx); // Останавливаем сервер
	mg_exit_library ();

	logger << "Сервер COLLABORATION. остановлен" << std::endl;

	return 0;
}

