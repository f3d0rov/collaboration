
#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

#include <string>
#include <cctype> // std::tolower
#include <map>

#include <ctime>
#include <chrono>
#include <unistd.h>

#include <functional>

#include <openssl/sha.h>

#define LOG_FILE_PATH "collab-server.log"

std::string getCurrentTimeString();
void chdirToExecutableDirectory (std::string progPath);
std::string getFileExtension (std::string path);
std::string readFile (std::string path, std::ios_base::openmode mode = std::ios::in);

std::string lowercase (std::string s);
bool isAscii (std::string s);

std::string sha256 (std::string input);

template <class T> std::chrono::microseconds usElapsedFrom_hiRes (T start) {
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);
}


class Logger: private std::streambuf, public std::ostream {
		std::ofstream file;
		bool newline = true;
	public:
		Logger();
		~Logger();

		void init ();
		int overflow(int c) override;
		void log(char c);
};

extern Logger logger;

class ScopedProtector {
	private:
		std::function<void()> _f;
	public:
		ScopedProtector (std::function<void()> f);
		~ScopedProtector ();
};
