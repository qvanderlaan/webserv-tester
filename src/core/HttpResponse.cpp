#include "HttpResponse.hpp"
#include <sstream>
#include <strings.h>

std::string HttpResponse::getHeaders(std::string_view key) const noexcept
{
	for (const auto& [k, v] : headers)
	{
		if (strcasecmp(k.c_str(), std::string(key).c_str()) == 0)
			return (v);
	}
	return ("");
}

HttpResponse HttpResponse::parse(const std::string& rawPayload)
{
	HttpResponse res;
	res.raw = rawPayload;
	if (rawPayload.empty())
		return (res);

	std::istringstream stream(rawPayload);
	std::string statusLine;
	if (std::getline(stream, statusLine))
	{
		if (!statusLine.empty() && statusLine.back() == '\r')
			statusLine.pop_back();
		std::istringstream ls(statusLine);
		std::string version;
		ls >> version >> res.statusCode;
		std::getline(ls, res.statusText);
		if (!res.statusText.empty() && res.statusText[0] == ' ')
			res.statusText.erase(0, 1);
	}

	std::string line;
	while (std::getline(stream, line) && line != "\r" && !line.empty())
	{
		if (line.back() == '\r')
			line.pop_back();
		auto colon = line.find(':');
		if (colon != std::string::npos)
		{
			std::string k = line.substr(0, colon);
			std::string v = line.substr(colon + 1);
			if (!v.empty() && v[0] == ' ')
				v.erase(0, 1);
			res.headers[k] = v;
		}
	}

	auto bodyPos = rawPayload.find("\r\n\r\n");
	if (bodyPos != std::string::npos)
		res.body = rawPayload.substr(bodyPos + 4);

	return (res);
}
