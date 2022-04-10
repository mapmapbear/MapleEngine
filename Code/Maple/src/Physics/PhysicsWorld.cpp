//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "PhysicsWorld.h"

namespace maple
{
	namespace physics
	{

		/*auto PhysicsWorld::exitPhysics() -> void
		{
			if (dynamicsWorld)
			{
				for (int32_t i = dynamicsWorld->getNumConstraints() - 1; i >= 0; i--)
				{
					dynamicsWorld->removeConstraint(dynamicsWorld->getConstraint(i));
				}
				for (int32_t i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
				{
					btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
					btRigidBody* body = btRigidBody::upcast(obj);
					if (body && body->getMotionState())
					{
						delete body->getMotionState();
					}
					dynamicsWorld->removeCollisionObject(obj);
					delete obj;
				}
			}
			//delete collision shapes
			for (int32_t j = 0; j < collisionShapes.size(); j++)
			{
				btCollisionShape* shape = collisionShapes[j];
				delete shape;
			}
			collisionShapes.clear();

			delete dynamicsWorld;
			dynamicsWorld = nullptr;

			delete solver;
			solver = nullptr;

			delete broadphase;
			broadphase = nullptr;

			delete dispatcher;
			dispatcher = nullptr;

			delete collisionConfiguration;
			collisionConfiguration = nullptr;
		}*/
	}
}