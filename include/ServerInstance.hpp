#pragma once

#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <signal.h>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

namespace fs = std::filesystem;

class ServerInstance
{
	private:
		fs::path _webservBin;
		fs::path _workingDir;
		std::string _configFile;
		pid_t _pid = -1;

	public:
		ServerInstance(fs::path webservBin, fs::path workingDir, std::string configFile = "main.conf");
		~ServerInstance(void);

		bool start(void);
		void stop(void);
};
