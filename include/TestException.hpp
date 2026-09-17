#pragma once

#include <exception>
#include <source_location>
#include <string>

class TestException : public std::exception
{
	private:
		std::string _msg;

	public:
		TestException(std::string msg, std::source_location loc = std::source_location::current());
		const char* what(void) const noexcept override;
};
