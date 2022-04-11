//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include <btBulletDynamicsCommon.h>
#include "Engine/Timestep.h"

namespace maple
{
	namespace physics 
	{
		namespace component
		{
			struct PhysicsWorld
			{
				btDefaultCollisionConfiguration* collisionConfiguration = nullptr;
				btDiscreteDynamicsWorld* dynamicsWorld = nullptr;
				btCollisionDispatcher* dispatcher = nullptr;
				btBroadphaseInterface* broadphase = nullptr;
				btConstraintSolver* solver = nullptr;
				btAlignedObjectArray<btCollisionShape*> collisionShapes;
			};
		};
		
		auto exitPhysics(component::PhysicsWorld& world) -> void;
	}
}