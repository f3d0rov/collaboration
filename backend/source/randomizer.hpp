#pragma once

#include <random>
#include <string>
#include <sstream>

typedef std::mt19937_64 PseudoRandomEngine;

// Hopeful
class Randomizer {
	private:
		PseudoRandomEngine _engine;
	public:
		Randomizer ();
		PseudoRandomEngine::result_type operator() ();
		std::string hex (int len);
};

extern Randomizer randomizer;
