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
		struct Hierarchy;
	}

	namespace global::component
	{
		struct SceneTransformChanged;
	}

	namespace hierarchy
	{
		auto MAPLE_EXPORT updateTransform(entt::entity entity, ecs::World world, global::component::SceneTransformChanged *transform = nullptr) -> void;

		auto MAPLE_EXPORT reset(component::Hierarchy &hy) -> void;
		// Return true if current entity is an ancestor of current entity
		//seems that the entt as a reference could have a bug....
		//TODO
		auto MAPLE_EXPORT compare(component::Hierarchy &left, entt::entity entity, ecs::World world) -> bool;
		//adjust the parent
		auto MAPLE_EXPORT reparent(entt::entity entity, entt::entity parent, component::Hierarchy &hierarchy, ecs::World world) -> void;

		auto MAPLE_EXPORT disconnectOnConstruct(std::shared_ptr<ExecutePoint> executePoint, bool disable) -> void;

		auto registerHierarchyModule(std::shared_ptr<ExecutePoint> executePoint) -> void;
	}        // namespace hierarchy
};           // namespace maple
