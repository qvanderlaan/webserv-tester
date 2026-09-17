// #include "TestRunner.hpp"
#include "TestRegistry.hpp"
#include <iostream>

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cerr << "Usage: " << argv[0] << " <webserv executable path>" << std::endl;
		return 1;
	}

	fs::path webservBin;
	try
	{
		webservBin = fs::canonical(argv[1]);
		if (!fs::is_regular_file(webservBin))
		{
			std::cerr << "[-] Error: " << webservBin << " is not a regular file." << std::endl;
			return 1;
		}
	}
	catch (const fs::filesystem_error& e)
	{
		std::cerr << "[-] Error resolving path: " << e.what() << std::endl;
		return 1;
	}

	const auto& tests = TestRegistry::instance().getTests();
	std::cout << "\n========== Webserv Test Suite (" << tests.size() << " tests found) ==========\n\n";

	int passedCount = 0;
	int failedCount = 0;

	for (const auto& test : tests)
	{
		std::cout << "[ RUN      ] " << test->getName() << " " << std::flush;
		auto start = std::chrono::high_resolution_clock::now();

		try
		{
			test->run(webservBin);
			auto end = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

			std::cout << "\r\033[32m[  PASSED  ]\033[0m " << test->getName() << " (" << duration << " ms)\n";
			passedCount++;
		}
		catch (const std::exception& e)
		{
			auto end = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

			std::cout << "\r\033[31m[  FAILED  ]\033[0m " << test->getName() << " (" << duration << " ms)\n";
			std::cout << "\033[31m\t↳ " << e.what() << "\033[0m\n";
			failedCount++;
		}
	}

	std::cout << "\n=======================================================\n";
	std::cout << "Results: \033[32m" << passedCount << " passed\033[0m, \033[31m" << failedCount << " failed\033[0m ("
			  << tests.size() << " total)\n\n";

	return (failedCount == 0) ? 0 : 1;
}