#include "TestRunner.hpp"
#include <iostream>

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << " <webserv_binary> [filter]\n";
		return 1;
	}

	try
	{
		fs::path bin = fs::canonical(argv[1]);
		std::string filter = (argc >= 3) ? argv[2] : "";
		TestRunner runner(bin, filter);
		return runner.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << "\n";
		return 1;
	}
}