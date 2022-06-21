//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include <memory>

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

	namespace global::physics::component
	{
		struct PhysicsWorld;
	}

	namespace physics
	{
		namespace component
		{
			struct Collider;
			struct RigidBody;
		};        // namespace component

		//auto MAPLE_EXPORT updateCollider(component::Collider &collider, global::physics::component::PhysicsWorld &world, component::RigidBody *rigidBody = nullptr) -> void;
		//auto MAPLE_EXPORT updateRigidBody(component::RigidBody &rigidBody, maple::component::Transform &transform, global::physics::component::PhysicsWorld &world, component::Collider *collider = nullptr) -> void;
		auto registerPhysicsModule(std::shared_ptr<ExecutePoint> executePoint) -> void;
	}        // namespace physics
}        // namespace maple