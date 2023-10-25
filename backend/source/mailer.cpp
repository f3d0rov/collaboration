
#include "mailer.hpp"

Mailer::Mailer () {

}

bool Mailer::init (std::string jsonConfigPath) { 
	this->readConfigFile (jsonConfigPath);
	this->_client.emplace (this->_server, this->_port);
	this->_client->setCredentials (jed_utils::cpp::Credential (this->_username, this->_password));
	this->_myAddress.emplace (this->_username.c_str(), this->_displayName.c_str());

	jed_utils::PlaintextMessage testMsg (
		this->_myAddress.value(),
		this->_myAddress.value(),
		"Startup test message",
		"Запущен сервер COLLABORATION."
	);

	int err = this->_client->sendMail (testMsg);
	if (err) {
		logger << "Ошибка SMTP #" << err << ": "<< this->_client->getErrorMessage (err) << std::endl << std::endl;
		logger << this->_client->getCommunicationLog () << std::endl;
		throw std::runtime_error ("Ошибка SMTP");
	}

	logger << "Успешно подключился к серверу SMTP " << this->_server << ":" << this->_port << " как " << this->_username << std::endl;

	return false;
}

void Mailer::readConfigFile (std::string jsonConfigPath) {
	std::ifstream f(jsonConfigPath);
	if (!f.is_open())
		throw std::logic_error (std::string("Не получилось открыть файл конфигурации smtp '") + jsonConfigPath + "'");

	nlohmann::json json;
	try {
		json = nlohmann::json::parse (f);
	} catch (nlohmann::json::exception &e) {
		throw std::logic_error (std::string("Не получилось обработать файл конфигурации smtp  '") + jsonConfigPath + "': " + e.what());
	}

	if (!(json.contains ("server") && json.contains ("port")
		&& json.contains ("username") && json.contains ("password")
		&& json.contains ("display_name")))
		throw std::logic_error (std::string("Не получилось обработать файл конфигурации smtp '") + jsonConfigPath + "': файл неправильно сформирован");

	this->_server = json["server"].get <std::string>();
	this->_port = json["port"].get <int>();
	this->_username = json["username"].get <std::string>();
	this->_password = json["password"].get <std::string>();
	this->_displayName = json["display_name"].get <std::string>();
}

Mailer mailer;
