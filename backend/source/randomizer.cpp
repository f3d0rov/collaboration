
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
	int i = 0;
	while (res.str().length() < len) {
		res << std::hex << (*this)();
	}

	return res.str().substr (0, len);
}

Randomizer randomizer;