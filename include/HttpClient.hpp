#pragma once

#include "HttpResponse.hpp"

class HttpClient
{
	private:
		std::string _host;
		int _port;
		HttpResponse sendRequest(const std::string& rawRequest);

	public:
		HttpClient(std::string host, int port);

		bool waitForServer(int maxTries = 40, int delayMs = 5);

		HttpResponse get(const std::string& path, const std::map<std::string, std::string>& headers);

		HttpResponse
		post(const std::string& path, const std::string& body, const std::map<std::string, std::string>& headers);

		HttpResponse deleteRequest(const std::string& path, const std::map<std::string, std::string>& headers);
};
