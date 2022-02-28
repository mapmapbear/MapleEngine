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
		auto init(entt::registry & registry) -> void;
		auto disconnectOnConstruct(bool disable, entt::registry & registry) -> void;
		auto update(entt::registry & registry) -> void;
		auto updateTransform(entt::entity entity, entt::registry & registry)-> void;
	private:
	};

};