//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "SceneGraph.h"
#include "Component/Component.h"
#include "Component/Transform.h"
#include "Component/MeshRenderer.h"

#include "Engine/Profiler.h"
namespace maple 
{
	auto SceneGraph::init(entt::registry& registry) -> void
	{
		registry.on_construct<component::Hierarchy>().connect<&component::Hierarchy::onConstruct>();
		registry.on_update<component::Hierarchy>().connect<&component::Hierarchy::onUpdate>();
		registry.on_destroy<component::Hierarchy>().connect<&component::Hierarchy::onDestroy>();
	}

	auto SceneGraph::disconnectOnConstruct(bool disable, entt::registry& registry)  -> void
	{
		if (disable)
			registry.on_construct<component::Hierarchy>().disconnect<&component::Hierarchy::onConstruct>();
		else
			registry.on_construct<component::Hierarchy>().connect<&component::Hierarchy::onConstruct>();
	}

	auto SceneGraph::update(entt::registry& registry)  -> void
	{
		PROFILE_FUNCTION();
		auto nonHierarchyView = registry.view<component::Transform>(entt::exclude<component::Hierarchy>);

		for (auto entity : nonHierarchyView)
		{
			registry.get<component::Transform>(entity).setWorldMatrix(glm::mat4{1.f});
		}

		auto view = registry.view<component::Transform, component::Hierarchy>();
		for (auto entity : view)
		{
			const auto hierarchy = registry.try_get<component::Hierarchy>(entity);
			if (hierarchy && hierarchy->getParent() == entt::null)
			{
				//Recursively update children
				updateTransform(entity, registry);
			}
		}

	}

	auto SceneGraph::updateTransform(entt::entity entity, entt::registry& registry)  -> void
	{
		PROFILE_FUNCTION();
		auto hierarchyComponent = registry.try_get<component::Hierarchy>(entity);
		if (hierarchyComponent)
		{
			auto transform = registry.try_get<component::Transform>(entity);
			if (transform)
			{
				if (hierarchyComponent->getParent() != entt::null)
				{
					auto parentTransform = registry.try_get<component::Transform>(hierarchyComponent->getParent());
					if (parentTransform)
					{
						transform->setWorldMatrix(parentTransform->getWorldMatrix());
					}
				}
				else
				{
					transform->setWorldMatrix(glm::mat4{1.f});
				}
			}

			entt::entity child = hierarchyComponent->getFirst();
			while (child != entt::null)
			{
				auto hierarchyComponent = registry.try_get<component::Hierarchy>(child);
				auto next = hierarchyComponent ? hierarchyComponent->getNext() : entt::null;
				updateTransform(child, registry);
				child = next;
			}
		}
	}
};



