
#include "utils.hpp"

std::string getCurrentTimeString() {
	std::time_t time = std::time({});
	char timeString[32];
	std::strftime( std::data(timeString), std::size(timeString),"%F %T%z", std::gmtime(&time));
	return timeString;
}

void chdirToExecutableDirectory (std::string progPath) {	
	std::string path = progPath.substr(0, progPath.find_last_of("\\/"));
	try {
		std::filesystem::current_path (path);
	} catch (std::exception &e) {
		std::cout << "Failed to change directory to '" << path << "': " << e.what() << std::endl;
	}
}

std::string getFileExtension (std::string path) {
	auto i = path.find_last_of ('.');
	if (i == path.npos) return "";
	return path.substr (i + 1);
}

std::string readFile (std::string path, std::ios_base::openmode mode) {
	std::ifstream in (path, std::ios::in | mode);
	std::stringstream ss;
	ss << in.rdbuf();
	return ss.str();
}

std::string sha256 (std::string str) {
	unsigned char hash[SHA256_DIGEST_LENGTH];

    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << ((int) hash[i]);
    }

    return ss.str();
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