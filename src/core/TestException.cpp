#include "TestException.hpp"
#include <filesystem>
#include <format>

TestException::TestException(std::string msg, std::source_location loc)
{
	this->_msg = std::format("{}:{}: {}", std::filesystem::path(loc.file_name()).filename().string(), loc.line(), msg);
}

const char* TestException::what(void) const noexcept
{
	return (this->_msg.c_str());
}
