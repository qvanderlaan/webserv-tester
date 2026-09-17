#pragma once

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

class ITestCase
{
	public:
		virtual ~ITestCase(void) = default;

		[[nodiscard]]
		virtual std::string getName(void) const = 0;

		virtual void run(const fs::path& webservBin) = 0;
};
