#pragma once

#include <map>
#include <string>
#include <string_view>

class HttpRequest
{
	public:
		std::string method = "GET";
		std::string path = "/";
		std::map<std::string, std::string> headers;
		std::string body;

		HttpRequest& setMethod(std::string m);
		HttpRequest& setPath(std::string p);
		HttpRequest& setHeader(std::string k, std::string v);
		HttpRequest& setBody(std::string b);

		[[nodiscard]] std::string build(std::string_view host, int port) const;
};
