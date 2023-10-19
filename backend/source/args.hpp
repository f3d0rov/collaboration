#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <initializer_list>
#include <stdexcept>
#include <iomanip>

struct ArgOption {
	std::string fullName, shortName;
	std::string description;
	bool hasParameter = false;

	ArgOption(const std::string &shortName, const std::string &fullName, const std::string &description, bool hasParameter = false);
};

class ArgsParser {
		std::map<std::string, int> _optionMap;
		std::vector<ArgOption> _optionList;
		std::string _execPath;

		std::map<std::string, std::string> _parsedOptions;
	public:
		ArgsParser();
		ArgsParser(std::initializer_list<ArgOption> options);
		void setupOptions(std::initializer_list<ArgOption> options);

		int parseArgs(const int argc, const char *argv[]);
		
		std::string getArgValue(const std::string &fullName);
		std::string getArgValue(const std::string &fullName, std::string defaultValue);
		bool hasArg(const std::string &fullName);
		std::string getExecPath();

		void printHelp();
};

