#include "Sandbox.hpp"
#include <fstream>
#include <stdexcept>
#include <unistd.h>

Sandbox::Sandbox(void)
{
	char tempTemplate[] = "/tmp/webserv_test_XXXXXX";
	char* dir = mkdtemp(tempTemplate);
	if (!dir)
		throw std::runtime_error("Failed to create temporary sandbox");
	this->_path = dir;
}

Sandbox::Sandbox(fs::path existingPath)
	: _path(fs::canonical(existingPath))
	, _isTemp(false)
{
	//
}

Sandbox::~Sandbox(void)
{
	if (this->_isTemp && !this->_path.empty() && fs::exists(this->_path))
		fs::remove_all(this->_path);
}

Sandbox::Sandbox(Sandbox&& other) noexcept
	: _path(std::move(other._path))
	, _isTemp(other._isTemp)
{
	other._isTemp = false;
}

Sandbox& Sandbox::operator=(Sandbox&& other) noexcept
{
	if (this != &other)
	{
		if (this->_isTemp && !this->_path.empty() && fs::exists(this->_path))
			fs::remove_all(_path);
		this->_path = std::move(other._path);
		this->_isTemp = other._isTemp;
		other._isTemp = false;
	}
	return (*this);
}

const fs::path& Sandbox::getPath(void) const noexcept
{
	return (this->_path);
}

void Sandbox::writeFile(const std::string& relativePath, const std::string& content)
{
	fs::path fullPath = _path / relativePath;
	fs::create_directories(fullPath.parent_path());
	std::ofstream ofs(fullPath, std::ios::binary);
	ofs << content;
}

void Sandbox::writeConfig(const std::string& content, const std::string& filename)
{
	writeFile(filename, content);
}
