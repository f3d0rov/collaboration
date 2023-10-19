
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

std::string mimeForExtension (std::string ext) {
	const std::map <std::string, std::string> table = {
		{"html", "text/html"},
		{"ico", "image/vnd.microsoft.icon"},
		{"bin", "application/octet-stream"},
		{"bmp", "image/bmp"},
		{"css", "text/css"},
		{"csv", "text/csv"},
		{"doc", "application/msword"},
		{"docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
		{"gif", "image/gif"},
		{"jpeg", "image/jpeg"},
		{"jpg", "image/jpeg"},
		{"js", "text/javascript"},
		{"json", "application/json"},
		{"png", "image/png"},
		{"pdf", "image/pdf"},
		{"svg", "image/svg+xml"},
		{"tif", "image/tiff"},
		{"tiff", "image/tiff"},
		{"txt", "text/plain"},
		{"webp", "image/webp"},
		{"xml", "application/xml"},
		{"zip", "application/zip"},
		{"7z", "application/x-7z-compressed"}
	};

	if (table.find (ext) == table.end()) return "application/octet-stream";
	return table.at (ext);
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