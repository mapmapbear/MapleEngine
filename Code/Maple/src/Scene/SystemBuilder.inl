//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Animation/AnimationSystem.h"
#include "2d/SpriteSystem.h"
#include "Scripts/Mono/MonoSystem.h"
#include "Physics/PhysicsSystem.h"
#include "Scene/System/HierarchyModule.h"
#include "Engine/Mesh.h"


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
}
