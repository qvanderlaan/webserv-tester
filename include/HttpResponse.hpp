#pragma once

#include <map>
#include <string>

struct HttpResponse
{
		int statusCode = 0;
		std::string statusText;
		std::map<std::string, std::string> headers;
		std::string body;
		std::string rawRequest;

		[[nodiscard]]
		std::string getHeaders(std::string& key) const noexcept;
};
