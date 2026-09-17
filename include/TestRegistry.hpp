#pragma once

#include "TestCase.hpp"
#include <vector>

class TestRegistry
{
	private:
		std::vector<TestCase> _tests;
		TestRegistry(void) = default;

	public:
		static TestRegistry& instance(void);
		void addTest(std::string name, std::function<void(TestContext&)> fn);
		[[nodiscard]] const std::vector<TestCase>& getTests(void) const;
};

#define TEST_CONCAT_IMPL(a, b) a##b
#define TEST_CONCAT(a, b) TEST_CONCAT_IMPL(a, b)

#define TEST_CASE_IMPL(name, id)                                                                                       \
	static void TEST_CONCAT(test_fn_, id)(TestContext & ctx);                                                          \
	namespace                                                                                                          \
	{                                                                                                                  \
	struct TEST_CONCAT(AutoReg_, id)                                                                                   \
	{                                                                                                                  \
			TEST_CONCAT(AutoReg_, id)()                                                                                \
			{                                                                                                          \
				TestRegistry::instance().addTest(name, &TEST_CONCAT(test_fn_, id));                                    \
			}                                                                                                          \
	};                                                                                                                 \
	[[maybe_unused]] static const TEST_CONCAT(AutoReg_, id) TEST_CONCAT(autoreg_, id);                                 \
	}                                                                                                                  \
	static void TEST_CONCAT(test_fn_, id)(TestContext & ctx)

#define TEST_CASE(name) TEST_CASE_IMPL(name, __COUNTER__)
