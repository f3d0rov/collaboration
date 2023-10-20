
#include "randomSearchPromptResource.hpp"


RandomSearchPromptResource::RandomSearchPromptResource (mg_context* ctx, std::string uri):
ApiResource (ctx, uri), randomGenerator (std::chrono::system_clock::now().time_since_epoch().count()) {

}

struct TemporaryRandomSearchPromptOption {
	std::string text, url;
};

ApiResponse RandomSearchPromptResource::processRequest(std::string method, std::string uri, nlohmann::json body) {
	if (method != "GET") return ApiResponse (405);
	const std::vector <TemporaryRandomSearchPromptOption> options = {
		{"Nine Inch Nails", "/b?id=123"},
		{"Трент Резнор", "/p?id=23"},
		{"How To Destroy Angels", "/b?id=223"}
	};
	auto option = options [this->randomGenerator() % options.size()];
	
	return ApiResponse ({{"text", option.text}, {"url", option.url}});
}
