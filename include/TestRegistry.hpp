#pragma once

#include "ITestCase.hpp"
#include <memory>
#include <vector>

class TestRegistry
{
	private:
		std::vector<std::unique_ptr<ITestCase>> _tests;
		TestRegistry(void) = default;

	public:
		static TestRegistry& instance(void);
		void registerTest(std::unique_ptr<ITestCase> test);
		const std::vector<std::unique_ptr<ITestCase>>& getTests(void) const;
};

#define REGISTER_TEST(TestClass)                                                                                       \
	static struct AutoReg_##TestClass                                                                                  \
	{                                                                                                                  \
			AutoReg_##TestClass(void)                                                                                  \
			{                                                                                                          \
				TestRegistry::instance().registerTest(std::make_unique<TestClass>());                                  \
			}                                                                                                          \
	} global_AutoReg_##TestClass;
