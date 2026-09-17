#include "HttpClient.hpp"
#include <arpa/inet.h>
#include <format>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/time.h>
#include <thread>
#include <unistd.h>

HttpClient::HttpClient(std::string host, int port)
	: _host(std::move(host))
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
			continue;

		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(_port);
		inet_pton(AF_INET, _host.c_str(), &addr.sin_addr);

		if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == 0)
		{
			close(sock);
			return (true);
		}
		close(sock);
		std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
	}
	return (false);
}

HttpResponse HttpClient::sendRaw(const std::string& rawPayload)
{
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		throw std::runtime_error("Failed to create client socket");

	struct timeval tv{.tv_sec = 2, .tv_usec = 0};
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_port);
	inet_pton(AF_INET, _host.c_str(), &addr.sin_addr);

	if (connect(sock, (sockaddr*)&addr, sizeof(addr)) != 0)
	{
		close(sock);
		throw std::runtime_error(std::format("Connection failed to {}:{}", _host, _port));
	}

	size_t sent = 0;
	while (sent < rawPayload.size())
	{
		ssize_t n = ::send(sock, rawPayload.data() + sent, rawPayload.size() - sent, 0);
		if (n <= 0)
		{
			close(sock);
			throw std::runtime_error("Send failed");
		}
		sent += n;
	}

	std::string responseData;
	char buf[4096];
	while (true)
	{
		ssize_t n = recv(sock, buf, sizeof(buf), 0);
		if (n <= 0)
			break;
		responseData.append(buf, n);
	}
	close(sock);

	return (HttpResponse::parse(responseData));
}

HttpResponse HttpClient::send(const HttpRequest& req)
{
	return (sendRaw(req.build(_host, _port)));
}

HttpResponse HttpClient::get(const std::string& path, std::map<std::string, std::string> headers)
{
	HttpRequest req;
	req.setMethod("GET").setPath(path);
	req.headers = std::move(headers);

	return (send(req));
}

HttpResponse HttpClient::post(const std::string& path, std::string body, std::map<std::string, std::string> headers)
{
	HttpRequest req;
	req.setMethod("POST").setPath(path).setBody(std::move(body));
	req.headers = std::move(headers);

	return (send(req));
}

HttpResponse HttpClient::deleteReq(const std::string& path, std::map<std::string, std::string> headers)
{
	HttpRequest req;
	req.setMethod("DELETE").setPath(path);
	req.headers = std::move(headers);

	return (send(req));
}
