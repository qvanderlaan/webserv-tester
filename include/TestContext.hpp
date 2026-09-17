#pragma once

#include "ServerInstance.hpp"
#include <filesystem>
#include <string>

class TestContext
{
	public:
		std::filesystem::path webservBin;

		ServerInstance spawnServer(const std::string& configTemplate, int customPort = -1);
};
