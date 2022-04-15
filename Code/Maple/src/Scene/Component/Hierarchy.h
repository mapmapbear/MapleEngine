//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Engine/Core.h"
#include "ecs/World.h"

namespace maple
{
	class ExecutePoint;

	namespace component
	{
		struct  Hierarchy
		{
			entt::entity parent = entt::null;
			entt::entity first = entt::null;
			entt::entity next = entt::null;
			entt::entity prev = entt::null;
		};
	}
};
