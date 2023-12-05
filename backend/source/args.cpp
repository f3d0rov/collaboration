
#include "args.hpp"


ArgOption::ArgOption(const std::string &shortName, const std::string &fullName, const std::string &description, bool hasParameter):
shortName(shortName), fullName(fullName), description(description), hasParameter(hasParameter) {

}


ArgsParser::ArgsParser() {

}

ArgsParser::ArgsParser(std::initializer_list<ArgOption> options) {
	this->setupOptions(options);
}

void ArgsParser::setupOptions(std::initializer_list<ArgOption> options) {
	for (auto &i: options) {
		if (this->_optionMap.find(std::string("--") + i.fullName) == this->_optionMap.end() && this->_optionMap.find(std::string("-") + i.shortName) == this->_optionMap.end()) {
			this->_optionList.push_back(i);
			if (i.fullName != "")
				this->_optionMap[std::string("--") + i.fullName] = this->_optionList.size() - 1;
			if (i.shortName != "")
				this->_optionMap[std::string("-") + i.shortName] = this->_optionList.size() - 1;
		} else {
			throw std::logic_error(std::string("ArgsParser has an option name collision - parameter name '--") + i.fullName + "' or '-" + i.shortName + "' appears multiple times.");
		}
	}
}

int ArgsParser::parseArgs(const int argc, const char *argv[]) {
	this->_execPath = argv[0];
	for (int i = 1; i < argc; i++) {
		auto elem = this->_optionMap.find(argv[i]);
		
		if (elem == this->_optionMap.end()) {
			std::cout << "No such argument: " << argv[i] << std::endl;
			return -1;
		}

		const ArgOption &arg = this->_optionList[elem->second];
		if (arg.hasParameter) {
			if (i + 1 == argc || std::string(argv[i + 1]).rfind("-", 0) != std::string::npos) {
				std::cout << "Missing parameter for argument " << argv[i] << std::endl;
				return -1;
			}

			std::string parameter = argv[i + 1];
			i += 1;
			
			this->_parsedOptions[arg.fullName] = parameter;
		} else {
			this->_parsedOptions[arg.fullName] = "";
		}
	}

	return 0;
}

std::string ArgsParser::getArgValue(const std::string &fullName) {
	return this->_parsedOptions.at(fullName);
}

std::string  ArgsParser::getArgValue(const std::string &fullName, std::string defaultValue) {
	if (this->_parsedOptions.find(fullName) != this->_parsedOptions.end()) return this->_parsedOptions[fullName];
	return defaultValue;
}

bool ArgsParser::hasArg(const std::string &fullName) {
	return this->_parsedOptions.find(fullName) != this->_parsedOptions.end();
}

std::string ArgsParser::getExecPath() {
	return this->_execPath;
}

void ArgsParser::printHelp() {
	std::cout << "Доступные опции:" << std::endl;

	for (auto i: this->_optionList) {
		if (i.shortName != "") {
			std::cout << std::setfill(' ') << std::setw(25) << ("--" + i.fullName + ", -" + i.shortName) << " | " << (" " + i.description) << std::endl; 
		} else {
			std::cout << std::setfill(' ') << std::setw(25) << ("--" + i.fullName) << " | " << (" " + i.description) << std::endl; 
		}
	}
}
