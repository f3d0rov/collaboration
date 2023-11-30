
#include "randomizer.hpp"

Randomizer::Randomizer () {
	std::random_device trulyRandomDevice; // Hopefully
	this->_engine.seed (trulyRandomDevice());
}

PseudoRandomEngine::result_type Randomizer::operator() () {
	return this->_engine();
}

std::string Randomizer::hex (int len) {
	std::stringstream res;
	
	while (res.str().length() < len) {
		res << std::hex << (*this)();
	}

	return res.str().substr (0, len);
}

std::string Randomizer::str (int len) {
	const char minChar = 33, maxChar = 126;

	std::string res;
	while (res.length() < len) {
		auto r = (*this)();
		char a = minChar + r % (maxChar - minChar + 1);
		res += a;
	}
	return res;
}

Randomizer randomizer;