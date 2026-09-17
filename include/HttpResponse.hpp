#pragma once

#include <map>
#include <string>
#include <string_view>

struct HttpResponse
{
		int statusCode = 0;
		std::string statusText;
		std::map<std::string, std::string> headers;
		std::string body;
		std::string raw;

		[[nodiscard]]
		std::string getHeaders(std::string_view key) const noexcept;

		static HttpResponse parse(const std::string& rawPayload);
};
