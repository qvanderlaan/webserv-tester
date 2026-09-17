#include "HttpRequest.hpp"
#include <format>

HttpRequest& HttpRequest::setMethod(std::string m)
{
	method = std::move(m);
	return (*this);
}
HttpRequest& HttpRequest::setPath(std::string p)
{
	path = std::move(p);
	return (*this);
}
HttpRequest& HttpRequest::setHeader(std::string k, std::string v)
{
	headers[std::move(k)] = std::move(v);
	return (*this);
}
HttpRequest& HttpRequest::setBody(std::string b)
{
	body = std::move(b);
	return (*this);
}

std::string HttpRequest::build(std::string_view host, int port) const
{
	std::string req = std::format("{} {} HTTP/1.1\r\n", method, path);

	if (!headers.contains("Host"))
		req += std::format("Host: {}:{}\r\n", host, port);

	if (!headers.contains("Connection"))
		req += "Connection: close\r\n";

	if (!body.empty() && !headers.contains("Content-Length"))
		req += std::format("Content-Length: {}\r\n", body.size());

	for (const auto& [k, v] : headers)
		req += std::format("{}: {}\r\n", k, v);

	return (req + "\r\n" + body);
}
