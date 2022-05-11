//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Engine/Timestep.h"
#include <btBulletDynamicsCommon.h>

namespace maple
{
	namespace global::physics
	{
		namespace component
		{
			struct PhysicsWorld
			{
				btDefaultCollisionConfiguration *        collisionConfiguration = nullptr;
				btDiscreteDynamicsWorld *                dynamicsWorld          = nullptr;
				btCollisionDispatcher *                  dispatcher             = nullptr;
				btBroadphaseInterface *                  broadphase             = nullptr;
				btConstraintSolver *                     solver                 = nullptr;
				btAlignedObjectArray<btCollisionShape *> collisionShapes;
			};
		};        // namespace component
	}             // namespace global::physics
	namespace physics
	{
		auto exitPhysics(global::physics::component::PhysicsWorld &world) -> void;
	}
}        // namespace maple