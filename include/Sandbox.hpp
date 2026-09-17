#pragma once

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

class Sandbox
{
	private:
		fs::path _path;
		bool _isTemp = true;

	public:
		Sandbox(void);
		explicit Sandbox(fs::path existingPath);
		~Sandbox(void);

		Sandbox(const Sandbox&) = delete;
		Sandbox& operator=(const Sandbox&) = delete;
		Sandbox(Sandbox&& other) noexcept;
		Sandbox& operator=(Sandbox&& other) noexcept;

		[[nodiscard]] const fs::path& getPath(void) const noexcept;
		void writeFile(const std::string& relativePath, const std::string& content);
		void writeConfig(const std::string& content, const std::string& filename = "webserv.conf");
};
