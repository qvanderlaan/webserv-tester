#include "TestRunner.hpp"
#include <iostream>

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cerr << "Usage: " << argv[0] << " <webserv executable path>" << std::endl;
		return (1);
	}

	TestRunner runner(argv[1]);
	return (0);
}