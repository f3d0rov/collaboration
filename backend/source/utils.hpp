
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
#include <thread>
#include <mutex>

#include <openssl/evp.h>
#include <openssl/sha.h>

#define LOG_FILE_PATH "collab-server.log"

using namespace std::literals::string_literals; // "123"s <=> std::string("123")


std::string getCurrentTimeString();
void chdirToExecutableDirectory (std::string progPath);
std::string getFileExtension (std::string path);
std::string readFile (std::string path, std::ios_base::openmode mode = std::ios::in);

std::string cookieString (std::string name, std::string value, bool httpOnly, long long maxAge);
std::string queryString (std::string from);

std::string lowercase (std::string s);
bool isAscii (std::string s);
std::string trimmed (std::string s);
std::string escapeHTML (std::string s);

std::string sha3_256 (std::string input);

template <class T> std::chrono::microseconds usElapsedFrom_hiRes (T start) {
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start);
}

std::string floatStr (double x, int precision = 3);
std::string prettyMicroseconds (std::chrono::microseconds);


class Logger;
class ScopedProtector;
class Common;


class Logger: private std::streambuf, public std::ostream {
		std::ofstream file;
		bool newline = true;
		int _sinceNewline = 0;
		bool _lineOverflow = false;
	public:
		std::mutex mutex;

		Logger();
		~Logger();

		void init ();
		int overflow(int c) override;
		void log(char c);
};

extern Logger _logger;

class LoggerLock {
	private:
		std::unique_lock <std::mutex> _lock;
	public:
		LoggerLock ();
		~LoggerLock ();

		template <class T>
		LoggerLock &operator<< (const T &x) {
			_logger << x;
			return *this;
		}

		LoggerLock &operator<< (std::ostream  &(*x)(std::ostream&));
};

#define logger (LoggerLock())

class ScopedProtector {
	private:
		std::function<void()> _f;
	public:
		ScopedProtector (std::function<void()> f);
		~ScopedProtector ();
};


class Common {
	public:
		std::string domain;
		std::string http = "http";
		std::string frontendDir;
		bool logSql;
		int logOverflowLineLength = 120;
};

extern Common common;


class RequestTooBigException: public std::runtime_error {
	public:
		RequestTooBigException (std::string w = "Принятый запрос превышает допустимый размер");
};


class UserMistakeException: public std::runtime_error {
	private:
		int _suggestedStatusCode;
	public:
		UserMistakeException (std::string w = "Пользовательская ошибка", int suggestedStatusCode = 400);
		int statusCode();
};
