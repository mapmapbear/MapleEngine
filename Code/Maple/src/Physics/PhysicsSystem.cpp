//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "PhysicsSystem.h"
#include "PhysicsWorld.h"
#include "Scene/Component/Component.h"
#include "Scene/System/ExecutePoint.h"
#include <ecs/ecs.h>


namespace maple
{
	namespace physics
	{
		namespace update 
		{
			using Entity = ecs::Chain
				::Write<component::PhysicsWorld>
				::Read<maple::component::DeltaTime>
				::To<ecs::Entity>;

			inline auto update(Entity entity, ecs::World world)
			{
				auto [phyWorld, dt] = entity;

				if (phyWorld.dynamicsWorld)
				{
					phyWorld.dynamicsWorld->stepSimulation(dt.dt);
				}
			}
		}

		auto registerPhysicsModule(std::shared_ptr<ExecutePoint> executePoint) -> void 
		{
			executePoint->registerGlobalComponent<component::PhysicsWorld>([](component::PhysicsWorld & world){
				world.collisionConfiguration = new btDefaultCollisionConfiguration();
				world.dispatcher = new btCollisionDispatcher(world.collisionConfiguration);//For parallel processing you can use a different dispatcher (see Extras/BulletMultiThreaded)
				world.broadphase = new btDbvtBroadphase();

				///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
				world.solver = new btSequentialImpulseConstraintSolver();
				world.dynamicsWorld = new btDiscreteDynamicsWorld(world.dispatcher, world.broadphase, world.solver, world.collisionConfiguration);
				world.dynamicsWorld->setGravity(btVector3(0, -9.8, 0));
			});
		}
	}
}