#pragma once

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include <map>
#include <string>

class HttpClient
{
	private:
		std::string _host;
		int _port;

	public:
		HttpClient(std::string host, int port);

		bool waitForServer(int maxRetries = 50, int delayMs = 10);
		HttpResponse send(const HttpRequest& req);
		HttpResponse sendRaw(const std::string& rawPayload);

		HttpResponse get(const std::string& path, std::map<std::string, std::string> headers = {});
		HttpResponse post(const std::string& path, std::string body, std::map<std::string, std::string> headers = {});
		HttpResponse deleteReq(const std::string& path, std::map<std::string, std::string> headers = {});
};
