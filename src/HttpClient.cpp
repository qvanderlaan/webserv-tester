#include "HttpClient.hpp"
#include "TestException.hpp"
#include <arpa/inet.h>
#include <format>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

HttpClient::HttpClient(std::string host, int port)
	: _host(host)
	, _port(port)
{
	//
}

bool HttpClient::waitForServer(int maxRetries, int delayMs)
{
	for (int i = 0; i < maxRetries; ++i)
	{
		int sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock < 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
			continue;
		}

		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(this->_port);
		inet_pton(AF_INET, _host.c_str(), &addr.sin_addr);

		if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0)
		{
			close(sock);
			return (true);
		}
		close(sock);
		std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
	}
	return (false);
}

HttpResponse HttpClient::sendRequest(const std::string& rawRequest)
{
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		throw TestException("Failed to create client socket.");

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_port);
	inet_pton(AF_INET, this->_host.c_str(), &addr.sin_addr);

	if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
	{
		close(sock);
		throw TestException("Failed to connect to " + this->_host + ":" + std::to_string(_port));
	}

	size_t sent = 0;
	while (sent < rawRequest.size())
	{
		ssize_t n = send(sock, rawRequest.data() + sent, rawRequest.size() - sent, 0);
		if (n <= 0)
		{
			close(sock);
			throw TestException("Failed while sending HTTP request data.");
		}
		sent += n;
	}

	HttpResponse res;
	char buffer[4096];
	while (true)
	{
		ssize_t n = recv(sock, buffer, sizeof(buffer), 0);
		if (n <= 0)
			break;
		res.rawRequest.append(buffer, n);
	}
	close(sock);

	if (res.rawRequest.empty())
		throw TestException("Empty HTTP response received.");

	// Parse Status Line
	std::istringstream stream(res.rawRequest);
	std::string statusLine;
	if (std::getline(stream, statusLine))
	{
		if (!statusLine.empty() && statusLine.back() == '\r')
			statusLine.pop_back();

		std::istringstream lineStream(statusLine);
		std::string httpVersion;
		lineStream >> httpVersion >> res.statusCode;
		std::getline(lineStream, res.statusText);
		if (!res.statusText.empty() && res.statusText[0] == ' ')
			res.statusText.erase(0, 1);
	}

	// Parse Headers
	std::string headerLine;
	while (std::getline(stream, headerLine) && headerLine != "\r" && !headerLine.empty())
	{
		if (headerLine.back() == '\r')
			headerLine.pop_back();

		auto colon = headerLine.find(':');
		if (colon != std::string::npos)
		{
			std::string key = headerLine.substr(0, colon);
			std::string val = headerLine.substr(colon + 1);
			if (!val.empty() && val[0] == ' ')
				val.erase(0, 1);
			res.headers[key] = val;
		}
	}

	// Body
	auto headerEnd = res.rawRequest.find("\r\n\r\n");
	if (headerEnd != std::string::npos)
		res.body = res.rawRequest.substr(headerEnd + 4);

	return (res);
}

HttpResponse HttpClient::get(const std::string& path, const std::map<std::string, std::string>& headers)
{
	std::string req =
		std::format("GET {} HTTP/1.1\r\nHost: {}:{}\r\nConnection: close\r\n", path, this->_host, this->_port);

	for (const auto& [k, v] : headers)
		req += std::format("{}: {}\r\n", k, v);
	req += "\r\n";
	return (this->sendRequest(req));
}

HttpResponse
HttpClient::post(const std::string& path, const std::string& body, const std::map<std::string, std::string>& headers)
{
	std::string req = std::format("POST {} HTTP/1.1\r\nHost: {}:{}\r\nContent-Length: {}\r\nConnection: close\r\n",
								  path, this->_host, this->_port, body.size());

	for (const auto& [k, v] : headers)
		req += std::format("{}: {}\r\n", k, v);
	req += "\r\n" + body;
	return (this->sendRequest(req));
}

HttpResponse HttpClient::deleteRequest(const std::string& path, const std::map<std::string, std::string>& headers)
{
	std::string req =
		std::format("DELETE {} HTTP/1.1\r\nHost: {}:{}\r\nConnection: close\r\n", path, this->_host, this->_port);

	for (const auto& [k, v] : headers)
		req += std::format("{}: {}\r\n", k, v);
	req += "\r\n";
	return (this->sendRequest(req));
}
