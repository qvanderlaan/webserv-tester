#include "TestRegistry.hpp"

TestRegistry& TestRegistry::instance(void)
{
	static TestRegistry reg;
	return (reg);
}

void TestRegistry::registerTest(std::unique_ptr<ITestCase> test)
{
	this->_tests.push_back(std::move(test));
}

const std::vector<std::unique_ptr<ITestCase>>& TestRegistry::getTests(void) const
{
	return (this->_tests);
}