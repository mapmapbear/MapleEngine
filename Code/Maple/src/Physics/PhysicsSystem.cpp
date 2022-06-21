//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "PhysicsSystem.h"
#include "Collider.h"
#include "PhysicsWorld.h"
#include "RigidBody.h"

#include "Others/Serialization.h"
#include "Scene/Component/AppState.h"
#include "Scene/Component/Component.h"
#include "Scene/Component/Transform.h"
#include "Scene/System/ExecutePoint.h"
#include "Scene/System/HierarchyModule.h"
#include <ecs/ecs.h>

namespace maple
{
	namespace physics
	{
		namespace update
		{
			using Entity = ecs::Registry ::Modify<global::physics::component::PhysicsWorld>::To<ecs::Entity>;

			inline auto updateWorld(Entity                              entity,
			                        const global::component::AppState * appState,
			                        const global::component::DeltaTime &dt,
			                        ecs::World                          world)
			{
				auto [phyWorld] = entity;

				if (appState && appState->state == EditorState::Play)
				{
					if (phyWorld.dynamicsWorld)
					{
						phyWorld.dynamicsWorld->stepSimulation(dt.dt);
					}
				}
			}

			using RigidBodyEntity = ecs::Registry ::Modify<component::RigidBody>::Modify<component::Collider>::Modify<maple::component::Transform>::To<ecs::Entity>;

			inline auto updateRigidbody(RigidBodyEntity entity, ecs::World world)
			{
				auto [rigidBody, collider, transform] = entity;

				if (world.getComponent<maple::global::component::AppState>().state == EditorState::Play)
				{
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

						transform.setLocalPosition(
						    Serialization::bulletToGlm(trans.getOrigin()) - collider.originalBox.center());

						transform.setLocalOrientation(
						    Serialization::bulletToGlm(trans.getRotation()));

						////////////////////////////////////////////////////////////////Debug//////////////////////////////////////////////////////////////////////
						rigidBody.localInertia    = Serialization::bulletToGlm(rigidBody.rigidbody->getLocalInertia());
						rigidBody.angularVelocity = Serialization::bulletToGlm(rigidBody.rigidbody->getAngularVelocity());
						rigidBody.velocity        = Serialization::bulletToGlm(rigidBody.rigidbody->getLinearVelocity());

						btVector3 aabbMin;
						btVector3 aabbMax;
						rigidBody.rigidbody->getAabb(aabbMin, aabbMax);
						collider.box = {
						    Serialization::bulletToGlm(aabbMin),
						    Serialization::bulletToGlm(aabbMax)};
					}
				}
			}
		}        // namespace update

		namespace on_game_start
		{
			using RigidBodyEntity = ecs::Registry ::Modify<component::RigidBody>::Modify<component::Collider>::Modify<maple::component::Transform>::To<ecs::Entity>;

			inline auto system(RigidBodyEntity entity, global::physics::component::PhysicsWorld &phyWorld, ecs::World world)
			{
				auto [rigidBody, collider, transform] = entity;

				if (collider.shape == nullptr)
				{
					switch (collider.type)
					{
						case ColliderType::BoxCollider:
							collider.shape = new btBoxShape(Serialization::glmToBullet(collider.box.size() / 2.f));
							break;
						case ColliderType::SphereCollider:
							collider.shape = new btSphereShape(collider.radius);
							break;
						case ColliderType::CapsuleCollider:
							collider.shape = new btCapsuleShape(collider.radius, collider.height);
							break;
						default:
							MAPLE_ASSERT(false, "Unsupported ColliderType");
					}

					if (collider.shape != nullptr)
					{
						phyWorld.collisionShapes.push_back(collider.shape);
					}
				}

				hierarchy::updateTransform(entity, world);

				if (rigidBody.rigidbody == nullptr)
				{
					btVector3 localInertia(0, 0, 0);
					if (rigidBody.mass != 0.f && !rigidBody.kinematic)
						collider.shape->calculateLocalInertia(rigidBody.mass, localInertia);

					btRigidBody::btRigidBodyConstructionInfo cInfo(
					    rigidBody.dynamic ? rigidBody.mass : 0.f, new btDefaultMotionState(btTransform(Serialization::glmToBullet(transform.getWorldOrientation()), Serialization::glmToBullet(transform.getWorldPosition() + collider.box.center()))), collider.shape, localInertia);

					collider.originalBox = collider.box;

					rigidBody.rigidbody = new btRigidBody(cInfo);

					if (rigidBody.mass == 0.f || !rigidBody.dynamic)
					{
						rigidBody.rigidbody->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT);
					}
					else
					{
						rigidBody.rigidbody->setCollisionFlags(btCollisionObject::CF_DYNAMIC_OBJECT);
					}

					if (rigidBody.kinematic)
					{
						rigidBody.rigidbody->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT | rigidBody.rigidbody->getCollisionFlags());
						rigidBody.rigidbody->setActivationState(DISABLE_DEACTIVATION);
					}

					phyWorld.dynamicsWorld->addRigidBody(rigidBody.rigidbody);
				}
			}
		}        // namespace on_game_start

		namespace on_game_ended
		{
			using RigidBodyEntity = ecs::Registry ::Modify<component::RigidBody>::Modify<component::Collider>::Modify<maple::component::Transform>::To<ecs::Entity>;

			inline auto system(RigidBodyEntity entity, global::physics::component::PhysicsWorld &phyWorld, ecs::World world)
			{
				auto [rigidBody, collider, transform] = entity;

				if (collider.shape != nullptr)
				{
					phyWorld.collisionShapes.remove(collider.shape);
					delete collider.shape;
					collider.shape = nullptr;
				}

				collider.box = collider.originalBox;

				if (rigidBody.rigidbody != nullptr)
				{
					phyWorld.dynamicsWorld->removeRigidBody(rigidBody.rigidbody);
					auto state = rigidBody.rigidbody->getMotionState();
					if (state != nullptr)
					{
						delete state;
					}
					delete rigidBody.rigidbody;
					rigidBody.rigidbody = nullptr;
				}
			}
		}        // namespace on_game_ended

		namespace init
		{
			inline auto initCollider(component::Collider &collider, Entity entity, ecs::World world)
			{
				if (collider.shape == nullptr)
				{
					switch (collider.type)
					{
						case ColliderType::BoxCollider:
							collider.box = {{-1, -1, -1}, {1, 1, 1}};
							break;
						case ColliderType::SphereCollider:
							collider.radius = 1.f;
							collider.box    = {{0, 0, 0}, {0, 0, 0}};
							break;
						case ColliderType::CapsuleCollider:
							collider.radius = 0.5f;
							collider.height = 2.f;
							collider.box    = {{0, 0, 0}, {0, 0, 0}};
							break;
					}
				}
			}

			inline auto initRigidBody(component::RigidBody &rigidRody, Entity entity, ecs::World world)
			{
				//rigidbody is dynamic if and only if mass is non zero, otherwise static
			}
		}        // namespace init

		namespace destory
		{
			inline auto destoryCollider(component::Collider &collider, Entity entity, ecs::World world)
			{
			}

			inline auto destoryRigidRody(component::RigidBody &rigidRody, Entity entity, ecs::World world)
			{
				if (rigidRody.rigidbody)
				{
					world.getComponent<global::physics::component::PhysicsWorld>().dynamicsWorld->removeRigidBody(rigidRody.rigidbody);
					btMotionState *ms = rigidRody.rigidbody->getMotionState();
					if (ms)
					{
						delete ms;
					}
					delete rigidRody.rigidbody;
				}
			}
		};        // namespace destory

		auto registerPhysicsModule(std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerGlobalComponent<global::physics::component::PhysicsWorld>([](auto &world) {
				world.collisionConfiguration = new btDefaultCollisionConfiguration();
				world.dispatcher             = new btCollisionDispatcher(world.collisionConfiguration);        //For parallel processing you can use a different dispatcher (see Extras/BulletMultiThreaded)
				world.broadphase             = new btDbvtBroadphase();

				///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
				world.solver        = new btSequentialImpulseConstraintSolver();
				world.dynamicsWorld = new btDiscreteDynamicsWorld(world.dispatcher, world.broadphase, world.solver, world.collisionConfiguration);
				world.dynamicsWorld->setGravity(btVector3(0, -9.8, 0));
			});

			executePoint->registerSystem<update::updateRigidbody>();
			executePoint->registerSystem<update::updateWorld>();

			executePoint->registerGameStart<on_game_start::system>();
			executePoint->registerGameEnded<on_game_ended::system>();

			executePoint->onConstruct<component::Collider, init::initCollider>();
			executePoint->onDestory<component::Collider, destory::destoryCollider>();

			executePoint->onConstruct<component::RigidBody, init::initRigidBody>();
			executePoint->onDestory<component::RigidBody, destory::destoryRigidRody>();
		}
	}        // namespace physics
}        // namespace maple