//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "PhysicsSystem.h"
#include "PhysicsWorld.h"
#include "Collider.h"
#include "RigidBody.h"

#include "Scene/Component/Component.h"
#include "Scene/Component/Transform.h"

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

			inline auto updateWorld(Entity entity, ecs::World world)
			{
				auto [phyWorld, dt] = entity;

				if (phyWorld.dynamicsWorld)
				{
					phyWorld.dynamicsWorld->stepSimulation(dt.dt);
				}
			}

			using RigidBodyEntity = ecs::Chain
				::Write<component::RigidBody>
				::Write<maple::component::Transform>
				::To<ecs::Entity>;

			inline auto updateRigidbody(RigidBodyEntity entity, ecs::World world)
			{
				auto [rigidBody,transform] = entity;

				if (rigidBody.dynamic && rigidBody.rigidbody)
				{
					btTransform trans;
					if (rigidBody.rigidbody && rigidBody.rigidbody->getMotionState())
					{
						rigidBody.rigidbody->getMotionState()->getWorldTransform(trans);
					}
					else
					{
						trans = rigidBody.rigidbody->getWorldTransform();
					}

					auto q = trans.getRotation();
					btScalar x = 0, y = 0, z = 0;
					q.getEulerZYX(z, y, x);

					const auto& btPos = trans.getOrigin();

					transform.setLocalPosition(glm::vec3{ btPos.x(), btPos.y(), btPos.z() });
					transform.setLocalOrientation(glm::vec3{ glm::degrees(x), glm::degrees(y), glm::degrees(z) });
				}
			}
		}

		namespace init 
		{
			inline auto initCollider(component::Collider & collider, Entity entity, ecs::World world)
			{
				switch (collider.type)
				{
				case ColliderType::BoxCollider:
					collider.shape = new btBoxShape({1,1,1});
					collider.box = { {-1,-1,-1}, { 1,1,1 } };
					break;
				case ColliderType::SphereCollider:
					collider.shape = new btSphereShape(1.f);
					break;
				}
				
				if (collider.shape != nullptr) 
				{
					world.getComponent<component::PhysicsWorld>().collisionShapes.push_back(collider.shape);
				}
			}

			inline auto initRigidBody(component::RigidBody& rigidRody, Entity entity, ecs::World world)
			{
				//rigidbody is dynamic if and only if mass is non zero, otherwise static
				rigidRody.dynamic = (rigidRody.mass != 0.f);
				auto& collider = entity.getComponent<component::Collider>();

				btVector3 localInertia(0, 0, 0);
				if (rigidRody.dynamic)
					collider.shape->calculateLocalInertia(rigidRody.mass, localInertia);

				btTransform transform;
				transform.setIdentity();

				btRigidBody::btRigidBodyConstructionInfo cInfo(rigidRody.mass, new btDefaultMotionState(transform), collider.shape, localInertia);

				rigidRody.rigidbody = new btRigidBody(cInfo);
				rigidRody.rigidbody->setUserIndex((int32_t)entity.getHandle());
				world.getComponent<component::PhysicsWorld>().dynamicsWorld->addRigidBody(rigidRody.rigidbody);
			}
		}

		namespace destory 
		{
			inline auto destoryCollider(component::Collider& collider, Entity entity, ecs::World world)
			{

			}

			inline auto destoryRigidRody(component::RigidBody& rigidRody, Entity entity, ecs::World world)
			{

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

			executePoint->registerSystem<update::updateRigidbody>();
			executePoint->registerSystem<update::updateWorld>();

			executePoint->onConstruct<component::Collider, init::initCollider>();
			executePoint->onDestory<component::Collider, destory::destoryCollider>();

			executePoint->onConstruct<component::RigidBody, init::initRigidBody>();
			executePoint->onDestory<component::RigidBody, destory::destoryRigidRody>();
		}
	}
}