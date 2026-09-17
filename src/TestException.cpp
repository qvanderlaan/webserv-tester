#include "TestException.hpp"
#include <filesystem>
#include <format>

namespace fs = std::filesystem;

TestException::TestException(std::string message, std::source_location loc)
{
	this->_message = std::format("{}:{}: {}", fs::path(loc.file_name()).filename().string(), loc.line(), message);
}

const char* TestException::what(void) const noexcept
{
	return (this->_message.c_str());
}
