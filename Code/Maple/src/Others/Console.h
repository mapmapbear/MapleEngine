//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include <iostream>
#include <string>
#include <stdarg.h>
#include <spdlog/spdlog.h>
#include "Engine/Core.h"
struct lua_State;

namespace maple 
{
	namespace LogExport
	{
		auto exportLua(lua_State* L) -> void;
	};

	class MAPLE_EXPORT Console
	{
	public:
		static auto init() -> void;
		static auto & getLogger() { return logger; }
	private:
		static std::shared_ptr<spdlog::logger> logger;
	};
};

#define LOGV(...)      maple::Console::getLogger()->trace(__VA_ARGS__)
#define LOGI(...)      maple::Console::getLogger()->info(__VA_ARGS__)
#define LOGW(...)      maple::Console::getLogger()->warn(__VA_ARGS__)
#define LOGE(...)      maple::Console::getLogger()->error(__VA_ARGS__)
#define LOGC(...)      maple::Console::getLogger()->critical(__VA_ARGS__)



