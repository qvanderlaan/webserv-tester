#include "TestRunner.hpp"
#include <iostream>

TestRunner::TestRunner(const char* webservLocation)
{
	try
	{
		this->_absoluteWebservLocation = fs::canonical(webservLocation);

		if (!fs::is_regular_file(this->_absoluteWebservLocation))
		{
			std::cerr << "Error: " << this->_absoluteWebservLocation << " is not a regular file." << std::endl;
			return;
		}

		std::cout << "Valid executable: " << this->_absoluteWebservLocation << std::endl;
	}
	catch (const fs::filesystem_error& e)
	{
		std::cerr << "Error resolving path: " << e.what() << std::endl;
	}
}