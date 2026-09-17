#include "TestRunner.hpp"
#include "TestRegistry.hpp"
#include <chrono>
#include <iostream>

TestRunner::TestRunner(std::filesystem::path webservBin, std::string filter)
	: _ctx{.webservBin = std::move(webservBin)}
	, _filter(std::move(filter))
{
	//
}

int TestRunner::run(void)
{
	const auto& tests = TestRegistry::instance().getTests();
	std::cout << "\n========== Webserv Test Suite ==========\n\n";

	int passed = 0, failed = 0, skipped = 0;

	for (const auto& test : tests)
	{
		if (!_filter.empty() && test.name.find(_filter) == std::string::npos)
		{
			skipped++;
			continue;
		}

		std::cout << "[ RUN      ] " << test.name << " " << std::flush;
		auto start = std::chrono::high_resolution_clock::now();

		try
		{
			test.fn(_ctx);
			auto ms =
				std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start)
					.count();
			std::cout << "\r\033[32m[  PASSED  ]\033[0m " << test.name << " (" << ms << " ms)\n";
			passed++;
		}
		catch (const std::exception& e)
		{
			auto ms =
				std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start)
					.count();
			std::cout << "\r\033[31m[  FAILED  ]\033[0m " << test.name << " (" << ms << " ms)\n";
			std::cout << "\033[31m\t↳ " << e.what() << "\033[0m\n";
			failed++;
		}
	}

	std::cout << "\n=========================================\n";
	std::cout << "Results: \033[32m" << passed << " passed\033[0m, \033[31m" << failed << " failed\033[0m, " << skipped
			  << " skipped\n\n";

	return ((failed == 0) ? 0 : 1);
}
