#include "TestRegistry.hpp"

TestRegistry& TestRegistry::instance(void)
{
	static TestRegistry reg;
	return (reg);
}

void TestRegistry::addTest(std::string name, std::function<void(TestContext&)> fn)
{
	this->_tests.push_back({std::move(name), std::move(fn)});
}

const std::vector<TestCase>& TestRegistry::getTests(void) const
{
	return (this->_tests);
}
