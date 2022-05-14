//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "Scene/System/HierarchyModule.h"
#include "Scene/Component/Hierarchy.h"
#include "Scene/Component/Transform.h"
#include "Scene/Entity/Entity.h"
#include "Scene/System/ExecutePoint.h"

namespace maple
{
	namespace hierarchy
	{
		auto updateTransform(entt::entity entity, ecs::World world, global::component::SceneTransformChanged *changed) -> void
		{
			PROFILE_FUNCTION();
			auto hierarchyComponent = world.tryGetComponent<component::Hierarchy>(entity);
			if (hierarchyComponent)
			{
				auto transform = world.tryGetComponent<component::Transform>(entity);
				if (transform)
				{
					if (hierarchyComponent->parent != entt::null)
					{
						auto parentTransform = world.tryGetComponent<component::Transform>(hierarchyComponent->parent);
						if (parentTransform && (parentTransform->hasUpdated() || transform->isDirty()))
						{
							transform->setWorldMatrix(parentTransform->getWorldMatrix());
							if (changed)
							{
								changed->dirty = true;
								changed->entities.emplace_back(entity);
							}
						}
					}
					else        //no parent....root
					{
						if (transform->isDirty())
						{
							transform->setWorldMatrix(glm::mat4{1.f});
							if (changed)
							{
								changed->dirty = true;
								changed->entities.emplace_back(entity);
							}
						}
					}
				}

				entt::entity child = hierarchyComponent->first;
				while (child != entt::null)
				{
					auto hierarchyComponent = world.tryGetComponent<component::Hierarchy>(child);
					auto next               = hierarchyComponent ? hierarchyComponent->next : entt::null;
					updateTransform(child, world);
					child = next;
				}
			}
		}

		namespace update_hierarchy
		{
			// clang-format off
			using Entity = ecs::Chain 
				::Write<maple::component::Hierarchy>
				::Write<maple::component::Transform>
				::To<ecs::Entity>;
			// clang-format on
			inline auto system(Entity entity, global::component::SceneTransformChanged *sceneChanged, ecs::World world)
			{
				auto [hierarchy, transform] = entity;
				if (hierarchy.parent == entt::null)
				{
					//Recursively update children
					updateTransform(entity, world);
				}
			}
		}        // namespace update_hierarchy

		namespace update_none_hierarchy
		{
			// clang-format off
			using Entity = ecs::Chain 
				::Without<maple::component::Hierarchy>
				::Write<maple::component::Transform>
				::To<ecs::Entity>;
			// clang-format on
			inline auto system(Entity entity, global::component::SceneTransformChanged *sceneChanged, ecs::World world)
			{
				auto [transform] = entity;
				if (transform.isDirty())
				{
					sceneChanged->dirty = true;
					transform.setWorldMatrix(glm::mat4(1.f));
					sceneChanged->entities.emplace_back(entity);
				}
			}
		}        // namespace update_none_hierarchy

		namespace reset_update
		{
			// clang-format off
			using Entity = ecs::Chain 
				::Write<maple::component::Transform>
				::To<ecs::Entity>;
			// clang-format on
			inline auto system(Entity entity, global::component::SceneTransformChanged *sceneChanged, ecs::World world)
			{
				auto [transform] = entity;
				transform.setHasUpdated(false);
				if (sceneChanged)
				{
					sceneChanged->dirty = false;
					sceneChanged->entities.clear();
				}
			}
		}        // namespace reset_update

		//delegate method
		//update hierarchy components when hierarchy component is added
		inline auto onConstruct(component::Hierarchy &hierarchy, Entity entity, ecs::World world) -> void
		{
			if (hierarchy.parent != entt::null)
			{
				auto &parentHierarchy = world.getOrAddComponent<component::Hierarchy>(hierarchy.parent);

				if (parentHierarchy.first == entt::null)
				{
					parentHierarchy.first = entity;
				}
				else
				{
					auto prevEnt          = parentHierarchy.first;
					auto currentHierarchy = world.tryGetComponent<component::Hierarchy>(prevEnt);

					while (currentHierarchy != nullptr && currentHierarchy->next != entt::null)
					{
						prevEnt          = currentHierarchy->next;
						currentHierarchy = world.tryGetComponent<component::Hierarchy>(prevEnt);
					}
					currentHierarchy->next = entity;
					hierarchy.prev         = prevEnt;
				}
			}
		}

		inline auto onDestroy(component::Hierarchy &hierarchy, Entity entity, ecs::World world) -> void
		{
			if (hierarchy.prev == entt::null || !world.isValid(hierarchy.prev))
			{
				if (hierarchy.parent != entt::null && world.isValid(hierarchy.parent))
				{
					auto parentHierarchy = world.tryGetComponent<component::Hierarchy>(hierarchy.parent);
					if (parentHierarchy != nullptr)
					{
						parentHierarchy->first = hierarchy.next;
						if (hierarchy.next != entt::null)
						{
							auto nextHierarchy = world.tryGetComponent<component::Hierarchy>(hierarchy.next);
							if (nextHierarchy != nullptr)
							{
								nextHierarchy->prev = entt::null;
							}
						}
					}
				}
			}
			else
			{
				auto prevHierarchy = world.tryGetComponent<component::Hierarchy>(hierarchy.prev);
				if (prevHierarchy != nullptr)
				{
					prevHierarchy->next = hierarchy.next;
				}
				if (hierarchy.next != entt::null)
				{
					auto nextHierarchy = world.tryGetComponent<component::Hierarchy>(hierarchy.next);
					if (nextHierarchy != nullptr)
					{
						nextHierarchy->prev = hierarchy.prev;
					}
				}
			}
		}

		inline auto onUpdate(component::Hierarchy &hierarchy, Entity entity, ecs::World world) -> void
		{
			// if is the first child
			if (hierarchy.prev == entt::null)
			{
				if (hierarchy.parent != entt::null)
				{
					auto parent_hierarchy = world.tryGetComponent<component::Hierarchy>(hierarchy.parent);
					if (parent_hierarchy != nullptr)
					{
						parent_hierarchy->first = hierarchy.next;
						if (hierarchy.next != entt::null)
						{
							auto next_hierarchy = world.tryGetComponent<component::Hierarchy>(hierarchy.next);
							if (next_hierarchy != nullptr)
							{
								next_hierarchy->prev = entt::null;
							}
						}
					}
				}
			}
			else
			{
				auto prevHierarchy = world.tryGetComponent<component::Hierarchy>(hierarchy.prev);
				if (prevHierarchy != nullptr)
				{
					prevHierarchy->next = hierarchy.next;
				}
				if (hierarchy.next != entt::null)
				{
					auto next_hierarchy = world.tryGetComponent<component::Hierarchy>(hierarchy.next);
					if (next_hierarchy != nullptr)
					{
						next_hierarchy->prev = hierarchy.prev;
					}
				}
			}
		}

		auto reset(component::Hierarchy &hy) -> void
		{
			hy.parent = entt::null;
			hy.first  = entt::null;
			hy.next   = entt::null;
			hy.prev   = entt::null;
		}

		auto compare(component::Hierarchy &left, entt::entity rhs, ecs::World world) -> bool
		{
			if (rhs == entt::null || rhs == left.parent || rhs == left.prev)
			{
				return true;
			}

			if (left.parent == entt::null)
			{
				return false;
			}
			else
			{
				auto &thisParent = world.getComponent<component::Hierarchy>(left.parent);
				auto &rhsParent  = world.getComponent<component::Hierarchy>(rhs).parent;
				if (compare(thisParent, left.parent, world))
					return true;
			}
			return false;
		}

		//adjust the parent
		auto reparent(entt::entity entity, entt::entity parent, component::Hierarchy &hierarchy, ecs::World world) -> void
		{
			onDestroy(world.getComponent<component::Hierarchy>(entity), {entity, world.getRegistry()}, world);

			hierarchy.parent = entt::null;
			hierarchy.next   = entt::null;
			hierarchy.prev   = entt::null;

			if (parent != entt::null)
			{
				hierarchy.parent = parent;
				onConstruct(world.getComponent<component::Hierarchy>(entity), {entity, world.getRegistry()}, world);
			}
		}

		auto disconnectOnConstruct(std::shared_ptr<ExecutePoint> executePoint, bool disable) -> void
		{
			executePoint->onConstruct<component::Hierarchy, hierarchy::onConstruct>(disable);
		}

		auto registerHierarchyModule(std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->onConstruct<component::Hierarchy, hierarchy::onConstruct>();
			executePoint->onDestory<component::Hierarchy, hierarchy::onDestroy>();
			executePoint->onUpdate<component::Hierarchy, hierarchy::onUpdate>();

			executePoint->registerSystem<update_none_hierarchy::system>();
			executePoint->registerSystem<update_hierarchy::system>();
			executePoint->registerSystemInFrameEnd<reset_update::system>();
		}
	}        // namespace hierarchy
};           // namespace maple
