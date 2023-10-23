#pragma once

#include <random>
#include <chrono>
#include <vector>

#include "../api_resource.hpp"


class RandomSearchPromptResource: public ApiResource {
	private:
		std::mt19937_64 randomGenerator;
	public:
		RandomSearchPromptResource (mg_context* ctx, std::string uri);

		ApiResponse processRequest (RequestData &rd, nlohmann::json body) final;

};
