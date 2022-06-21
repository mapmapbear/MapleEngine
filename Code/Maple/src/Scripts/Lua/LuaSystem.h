//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Scene/System/ExecutePoint.h"

namespace maple
{
	namespace lua
	{
		auto registerLuaSystem(std::shared_ptr<ExecutePoint> executePoint) -> void;
	}
};        // namespace maple