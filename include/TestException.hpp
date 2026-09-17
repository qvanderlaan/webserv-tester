#pragma once

#include <source_location>
#include <string>

class TestException : public std::exception
{
	private:
		std::string _message;

	public:
		TestException(std::string message, std::source_location loc = std::source_location::current());

		const char* what(void) const noexcept override;
};