//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <memory>
#include "Engine/Core.h"


/**
 * in editor mode, the RigidBody and Collider should not be added into physical world
 * they should be added into world when the game begin.
 * so in the editor mode, we only care the parameters
 */

namespace maple
{
	class ExecutePoint;

	namespace component
	{
		class Transform;
	}

	namespace physics 
	{
		namespace component
		{
			struct Collider;
			struct RigidBody;
			struct PhysicsWorld;
		};

		auto MAPLE_EXPORT updateCollider(component::Collider& collider, component::PhysicsWorld & world, component::RigidBody* rigidBody = nullptr) -> void;
		auto MAPLE_EXPORT updateRigidBody(component::RigidBody& rigidBody, maple::component::Transform& transform, component::PhysicsWorld& world, component::Collider* collider = nullptr) -> void;

		auto registerPhysicsModule(std::shared_ptr<ExecutePoint> executePoint) -> void;
	}
}