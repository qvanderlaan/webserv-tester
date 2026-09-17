#include "ServerInstance.hpp"
#include <arpa/inet.h>
#include <fcntl.h>
#include <format>
#include <fstream>
#include <signal.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

int ServerInstance::findAvailablePort(void)
{
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		return (8080);

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	bind(sock, (sockaddr*)&addr, sizeof(addr));
	socklen_t len = sizeof(addr);
	getsockname(sock, (sockaddr*)&addr, &len);
	int port = ntohs(addr.sin_port);
	close(sock);
	return (port);
}

ServerInstance::ServerInstance(fs::path webservBin, Sandbox sandbox, std::string configFile, int port)
	: _webservBin(std::move(webservBin))
	, _sandbox(std::move(sandbox))
	, _configFile(std::move(configFile))
	, _port(port == -1 ? findAvailablePort() : port)
{
	this->_logFile = _sandbox.getPath() / "server.log";
}

ServerInstance::~ServerInstance(void)
{
	this->stop();
}

ServerInstance::ServerInstance(ServerInstance&& other) noexcept
	: _webservBin(std::move(other._webservBin))
	, _sandbox(std::move(other._sandbox))
	, _configFile(std::move(other._configFile))
	, _port(other._port)
	, _pid(other._pid)
	, _logFile(std::move(other._logFile))
{
	other._pid = -1;
}

ServerInstance& ServerInstance::operator=(ServerInstance&& other) noexcept
{
	if (this != &other)
	{
		this->stop();
		this->_webservBin = std::move(other._webservBin);
		this->_sandbox = std::move(other._sandbox);
		this->_configFile = std::move(other._configFile);
		this->_port = other._port;
		this->_pid = other._pid;
		this->_logFile = std::move(other._logFile);
		other._pid = -1;
	}
	return (*this);
}

void ServerInstance::start(void)
{
	this->_pid = fork();
	if (this->_pid < 0)
		throw std::runtime_error("Fork failed");

	if (this->_pid == 0)
	{
		int logFd = open(this->_logFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (logFd >= 0)
		{
			dup2(logFd, STDOUT_FILENO);
			dup2(logFd, STDERR_FILENO);
			close(logFd);
		}
		if (chdir(this->_sandbox.getPath().c_str()) != 0)
			_exit(1);
		execl(this->_webservBin.c_str(), this->_webservBin.c_str(), this->_configFile.c_str(), nullptr);
		_exit(1);
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	int status = 0;
	if (waitpid(_pid, &status, WNOHANG) != 0)
		throw std::runtime_error(std::format("Webserv exited unexpectedly. Logs:\n{}", getLogs()));
}

void ServerInstance::stop(void)
{
	if (this->_pid > 0)
	{
		kill(this->_pid, SIGTERM);
		int status = 0;
		for (int i = 0; i < 20; ++i)
		{
			if (waitpid(this->_pid, &status, WNOHANG) != 0)
			{
				this->_pid = -1;
				return;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		kill(this->_pid, SIGKILL);
		waitpid(this->_pid, &status, 0);
		this->_pid = -1;
	}
}

int ServerInstance::getPort(void) const noexcept
{
	return (this->_port);
}

Sandbox& ServerInstance::sandbox(void) noexcept
{
	return (this->_sandbox);
}

HttpClient ServerInstance::client(void) const
{
	return (HttpClient("127.0.0.1", this->_port));
}

std::string ServerInstance::getLogs(void) const
{
	std::ifstream ifs(this->_logFile);
	return (std::string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>()));
}