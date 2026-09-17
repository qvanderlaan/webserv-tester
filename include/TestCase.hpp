#pragma once

#include <functional>
#include <string>

class TestContext;

struct TestCase
{
		std::string name;
		std::function<void(TestContext&)> fn;
};
