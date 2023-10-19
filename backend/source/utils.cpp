
#include "utils.hpp"

std::string getCurrentTimeString() {
	std::time_t time = std::time({});
	char timeString[32];
	std::strftime( std::data(timeString), std::size(timeString),"%F %T%z", std::gmtime(&time));
	return timeString;
}

void chdirToExecutableDirectory (std::string progPath) {	
	std::string path = progPath.substr(0, progPath.find_last_of("\\/"));
	int err = chdir(path.c_str());
	if (err) {
		std::cout << "Failed to change directory to '" << path << "', chdir says '" << err << "'" << std::endl;
	}
}


Logger::Logger(): std::ostream(this) {

}

Logger::~Logger() {
	this->file.close();
}

void Logger::init () {
	this->file.open(LOG_FILE_PATH, std::ios::out | std::ios::app);
	if (!this->file.is_open()) {
		std::cout << "Failed to open log file " << LOG_FILE_PATH << std::endl;
	}
}

int Logger::overflow(int c) {
	this->log(c);
	return 0;
}

void Logger::log(char c) {
	if (newline) {
		auto ts = "[" + getCurrentTimeString() + "] ";
		std::cout << ts;
		this->file << ts;
		newline = false;
	}

	std::cout.put(c);
	this->file.put(c);
	
	if (c == '\n') {
		newline = true;
		this->file.flush();
	}
}

Logger logger;


ScopedProtector::ScopedProtector (std::function<void()> f) {
	this->_f = f;
}

ScopedProtector::~ScopedProtector () {
	this->_f();
}