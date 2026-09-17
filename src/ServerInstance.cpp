#include "ServerInstance.hpp"
#include "TestException.hpp"
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <thread>

namespace fs = std::filesystem;

ServerInstance::ServerInstance(fs::path webservBin, fs::path workingDir, std::string configFile)
	: _webservBin(std::move(webservBin))
	, _workingDir(std::move(workingDir))
	, _configFile(std::move(configFile))
{
}

ServerInstance::~ServerInstance(void)
{
	stop();
}

bool ServerInstance::start(void)
{
	this->_pid = fork();
	if (this->_pid < 0)
		throw TestException("Failed to fork webserv process.");

	if (this->_pid == 0)
	{
		// Silence stdout/stderr if wanted or redirect to /dev/null
		int devNull = open("/dev/null", O_WRONLY);
		if (devNull >= 0)
		{
			dup2(devNull, STDOUT_FILENO);
			close(devNull);
		}

		if (chdir(this->_workingDir.c_str()) != 0)
			_exit(1);

		execl(this->_webservBin.c_str(), this->_webservBin.c_str(), this->_configFile.c_str(), nullptr);
		_exit(1);
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	int status = 0;
	if (waitpid(_pid, &status, WNOHANG) != 0)
		throw TestException("Webserv exited unexpectedly on startup.");

	return true;
}

void ServerInstance::stop(void)
{
	if (this->_pid > 0)
	{
		kill(this->_pid, SIGTERM);
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		int status = 0;
		if (waitpid(this->_pid, &status, WNOHANG) == 0)
		{
			kill(this->_pid, SIGKILL);
			waitpid(this->_pid, &status, 0);
		}
		this->_pid = -1;
	}
}