//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <array>
#include <cstdint>
#include <map>
#include <memory.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#define MAPLE_ASSERT(condition, ...)                     \
	{                                                    \
		if (!(condition))                                \
		{                                                \
			LOGE("Assertion Failed : {0}", __VA_ARGS__); \
			__debugbreak();                              \
		}                                                \
	}

#ifdef PLATFORM_WINDOWS
#	pragma warning(disable : 4251)

#	ifdef MAPLE_DYNAMIC
#		ifdef MAPLE_ENGINE
#			define MAPLE_EXPORT __declspec(dllexport)
#		else
#			define MAPLE_EXPORT __declspec(dllimport)
#		endif
#	else
#		define MAPLE_EXPORT
#	endif

#	define MAPLE_HIDDEN

#else

#	define MAPLE_EXPORT __attribute__((visibility("default")))
#	define MAPLE_HIDDEN __attribute__((visibility("hidden")))

#endif

#define BIT(x) (1 << x)


#define NO_COPYABLE(TypeName) \
	TypeName(const TypeName &) = delete;   \
	TypeName(TypeName &&) = delete;	\
	TypeName& operator=(TypeName &&) = delete;	\
	TypeName& operator=(const TypeName &) = delete