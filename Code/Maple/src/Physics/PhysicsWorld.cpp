//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "PhysicsWorld.h"

namespace maple
{
	namespace physics
	{
		auto exitPhysics(global::physics::component::PhysicsWorld &world) -> void
		{
			if (world.dynamicsWorld)
			{
				for (int32_t i = world.dynamicsWorld->getNumConstraints() - 1; i >= 0; i--)
				{
					world.dynamicsWorld->removeConstraint(world.dynamicsWorld->getConstraint(i));
				}
				for (int32_t i = world.dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
				{
					btCollisionObject *obj  = world.dynamicsWorld->getCollisionObjectArray()[i];
					btRigidBody *      body = btRigidBody::upcast(obj);
					if (body && body->getMotionState())
					{
						delete body->getMotionState();
					}
					world.dynamicsWorld->removeCollisionObject(obj);
					delete obj;
				}
			}
			//delete collision shapes
			for (int32_t j = 0; j < world.collisionShapes.size(); j++)
			{
				btCollisionShape *shape = world.collisionShapes[j];
				delete shape;
			}
			world.collisionShapes.clear();

			delete world.dynamicsWorld;
			world.dynamicsWorld = nullptr;

			delete world.solver;
			world.solver = nullptr;

			delete world.broadphase;
			world.broadphase = nullptr;

			delete world.dispatcher;
			world.dispatcher = nullptr;

			delete world.collisionConfiguration;
			world.collisionConfiguration = nullptr;
		}
	}        // namespace physics
}        // namespace maple