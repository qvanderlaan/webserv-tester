#pragma once

#include "TestException.hpp"

#define TEST_ASSERT(cond)                                                                                              \
	do                                                                                                                 \
	{                                                                                                                  \
		if (!(cond))                                                                                                   \
			throw TestException("Assertion failed: " #cond);                                                           \
	} while (false)

#define TEST_ASSERT_EQ(actual, expected)                                                                               \
	do                                                                                                                 \
	{                                                                                                                  \
		auto _a = (actual);                                                                                            \
		auto _e = (expected);                                                                                          \
		if (_a != _e)                                                                                                  \
		{                                                                                                              \
			std::ostringstream _oss;                                                                                   \
			_oss << "Equality check failed: " #actual " == " #expected << "\n\tExpected: " << _e                       \
				 << "\n\tActual:   " << _a;                                                                            \
			throw TestException(_oss.str());                                                                           \
		}                                                                                                              \
	} while (false)

#define TEST_ASSERT_CONTAINS(big, small)                                                                               \
	do                                                                                                                 \
	{                                                                                                                  \
		if ((big).find(small) == std::string::npos)                                                                    \
		{                                                                                                              \
			std::ostringstream _oss;                                                                                   \
			_oss << "Substring not found: " #small " inside " #big << "\n\tLooking for: " << (small);                  \
			throw TestException(_oss.str());                                                                           \
		}                                                                                                              \
	} while (false)
