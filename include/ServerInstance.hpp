#pragma once

#include "HttpClient.hpp"
#include "Sandbox.hpp"
#include <filesystem>
#include <string>

class ServerInstance
{
	private:
		fs::path _webservBin;
		Sandbox _sandbox;
		std::string _configFile;
		int _port;
		pid_t _pid = -1;
		fs::path _logFile;

		static int findAvailablePort(void);

	public:
		ServerInstance(fs::path webservBin, Sandbox sandbox, std::string configFile = "webserv.conf", int port = -1);
		~ServerInstance(void);

		ServerInstance(const ServerInstance&) = delete;
		ServerInstance& operator=(const ServerInstance&) = delete;

		ServerInstance(ServerInstance&& other) noexcept;
		ServerInstance& operator=(ServerInstance&& other) noexcept;

		void start(void);
		void stop(void);

		[[nodiscard]] int getPort(void) const noexcept;
		[[nodiscard]] Sandbox& sandbox(void) noexcept;
		[[nodiscard]] HttpClient client(void) const;
		[[nodiscard]] std::string getLogs(void) const;
};
