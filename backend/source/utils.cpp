
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

std::string lowercase (std::string s) {
	std::string res = s;
	for (int i = 0; i < res.length(); i++) {
		res[i] = std::tolower (res[i]);
	}
	return res;
}

bool isAscii (std::string s) {
	for (int i = 0; i < s.length(); i++) {
		if (static_cast<unsigned char>(s[i]) > 127) return false;
	}
	return true;
}

std::string sha3_256 (std::string input) {;
	// const EVP_MD *EVP_sha3_256(void);
	EVP_MD_CTX *mdctx;
	if ((mdctx = EVP_MD_CTX_new()) == nullptr)
		throw std::runtime_error ("sha3_256: (mdctx = EVP_MD_CTX_new()) == nullptr");
	if (1 != EVP_DigestInit_ex(mdctx, EVP_sha3_256(), nullptr))
		throw std::runtime_error ("sha3_256: 1 != EVP_DigestInit_ex(mdctx, EVP_sha3_256(), nullptr)");
	if(1 != EVP_DigestUpdate(mdctx, input.c_str(), input.length()))
		throw std::runtime_error ("sha3_256: 1 != EVP_DigestInit_ex(mdctx, EVP_sha3_256(), nullptr)");
	unsigned char* digest = (unsigned char*) OPENSSL_malloc (EVP_MD_size (EVP_sha3_256()));
	if (digest == nullptr) throw ("sha3_256: digest == nullptr");
	unsigned int digest_len = 0;
	if (1 != EVP_DigestFinal_ex(mdctx, digest, &digest_len))
		throw ("sha3_256: 1 != EVP_DigestFinal_ex(mdctx, digest, &digest_len)");

	EVP_MD_CTX_free(mdctx);

	std::stringstream ss;
	for (int i = 0; i < EVP_MD_size(EVP_sha3_256()); i++) {
		ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
	}

	OPENSSL_free (digest);
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