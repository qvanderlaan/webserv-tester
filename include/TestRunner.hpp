#pragma once

#include "TestContext.hpp"
#include <string>

class TestRunner
{
	private:
		TestContext _ctx;
		std::string _filter;

	public:
		TestRunner(std::filesystem::path webservBin, std::string filter = "");
		int run(void);
};
