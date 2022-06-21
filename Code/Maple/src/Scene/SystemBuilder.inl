//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "2d/SpriteSystem.h"
#include "Animation/AnimationSystem.h"
#include "Physics/PhysicsSystem.h"
#include "Scene/Scene.h"
#include "Scene/System/HierarchyModule.h"
#include "Scripts/Mono/MonoSystem.h"

namespace maple
{
	inline auto registerSystem(std::shared_ptr<ExecutePoint> executePoint)
	{
		hierarchy::registerHierarchyModule(executePoint);
		animation::registerAnimationModule(executePoint);
		sprite2d::registerSprite2dModule(executePoint);
		mono::registerMonoModule(executePoint);
		physics::registerPhysicsModule(executePoint);
		mesh::registerMeshModule(executePoint);
	}
}        // namespace maple
