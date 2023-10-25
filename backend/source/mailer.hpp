#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <optional>

#include <smtpclient/cpp/opportunisticsecuresmtpclient.hpp>
#include "nlohmann-json/json.hpp"

#include "utils.hpp"

#define DEFAULT_SMTP_CONFIG_FILE "smtp-config.json"


typedef const std::unordered_map <std::string, std::string>& EmailSubstitutions;

class Mailer {
	private:
		std::optional <jed_utils::cpp::OpportunisticSecureSMTPClient> _client;
		std::optional <jed_utils::MessageAddress> _myAddress;
		std::mutex _clientMutex;

		int _port;
		std::string _server, _username, _password, _displayName;
		std::string _emailTemplatesFolderPath = "email-templates/";

		bool _started = false;
	public:
		
		Mailer ();
		void init (std::string jsonConfigPath);
		void readConfigFile (std::string jsonConfigPath);

		std::string openReadSubstitute (std::string path, EmailSubstitutions replace);
		void sendHtmlLetter (std::string destination, std::string subject, std::string path, EmailSubstitutions replace);
};

extern Mailer mailer;
