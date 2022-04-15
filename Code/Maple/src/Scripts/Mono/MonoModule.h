//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Engine/Core.h"
#include <memory>
#include <unordered_map>

namespace maple
{
	namespace component
	{
		struct MonoComponent;
	}

	namespace mono 
	{
		auto MAPLE_EXPORT addScript(component::MonoComponent & comp, const std::string& name,int32_t entity) -> void;
		auto MAPLE_EXPORT remove(component::MonoComponent& comp,const std::string& script) -> void;
	}
};        // namespace maple
