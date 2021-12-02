//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <string>
#include "Engine/Core.h"

#include <entt/entity/fwd.hpp>

namespace maple
{
	class MAPLE_EXPORT SceneGraph final
	{
	public:
		SceneGraph() = default;
		~SceneGraph() = default;
		void init(entt::registry & registry);
		void disconnectOnConstruct(bool disable, entt::registry & registry);
		void update(entt::registry & registry);
		void updateTransform(entt::entity entity, entt::registry & registry);
	};

};