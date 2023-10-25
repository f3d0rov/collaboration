
#include <string>
#include <map>
#include <mutex>
#include <optional>

#include <smtpclient/cpp/opportunisticsecuresmtpclient.hpp>
#include "nlohmann-json/json.hpp"

#include "utils.hpp"

#define DEFAULT_SMTP_CONFIG_FILE "smtp-config.json"


class Mailer {
	private:
		std::optional <jed_utils::cpp::OpportunisticSecureSMTPClient> _client;
		std::optional <jed_utils::MessageAddress> _myAddress;
		std::mutex _clientMutex;

		int _port;
		std::string _server, _username, _password, _displayName;
	public:
		
		Mailer ();
		bool init (std::string jsonConfigPath);
		void readConfigFile (std::string jsonConfigPath);

		bool sendHtmlFile (std::string path, std::map <std::string, std::string> replace);
};

extern Mailer mailer;
