//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Engine/Core.h"
#include <string>

struct lua_State;

namespace maple 
{
	class MAPLE_EXPORT LuaVirtualMachine final
	{
	public:
		LuaVirtualMachine();
		~LuaVirtualMachine();
		auto init() -> void;
		inline auto getState() { return L; }
	private:
		auto addSystemPath(const std::string& path) -> void;
		auto addPath(const std::string& path) -> void;
		lua_State * L = nullptr;
	};
};