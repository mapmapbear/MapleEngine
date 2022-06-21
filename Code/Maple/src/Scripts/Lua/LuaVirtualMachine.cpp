//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
extern "C"
{
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}
#include "ComponentExport.h"
#include "Devices/Input.h"
#include "Devices/InputExport.h"
#include "LuaComponent.h"
#include "LuaVirtualMachine.h"
#include "MathExport.h"
#include "Others/Console.h"
#include <LuaBridge/LuaBridge.h>
#include <filesystem>
#include <functional>

namespace maple
{
	LuaVirtualMachine::LuaVirtualMachine()
	{
	}

	LuaVirtualMachine::~LuaVirtualMachine()
	{
		lua_close(L);
	}

	auto LuaVirtualMachine::init() -> void
	{
		L = luaL_newstate();
		luaL_openlibs(L);        //load all default lua functions
		addPath(".");
		InputExport::exportLua(L);
		LogExport::exportLua(L);
		MathExport::exportLua(L);
		ComponentExport::exportLua(L);
	}

	auto LuaVirtualMachine::addSystemPath(const std::string &path) -> void
	{
		std::string v;
		lua_getglobal(L, "package");
		lua_getfield(L, -1, "path");
		v.append(lua_tostring(L, -1));
		v.append(";");
		v.append(path.c_str());
		lua_pushstring(L, v.c_str());
		lua_setfield(L, -3, "path");
		lua_pop(L, 2);
	}

	auto LuaVirtualMachine::addPath(const std::string &path) -> void
	{
		addSystemPath(path + "/?.lua");

		for (const auto &entry : std::filesystem::directory_iterator(path))
		{
			bool isDir = std::filesystem::is_directory(entry);
			if (isDir)
			{
				try
				{
					addPath(entry.path().string());
				}
				catch (...)
				{
					LOGW("{0} : catch an error", __FUNCTION__);
				}
			}
		}
	}

};        // namespace maple
