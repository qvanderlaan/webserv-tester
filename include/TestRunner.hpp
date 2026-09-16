#pragma once

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

class TestRunner
{
	private:
		fs::path _absoluteWebservLocation;

	public:
		TestRunner(const char* webservLocation);
};
