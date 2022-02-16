//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "EntityManager.h"
#include "Scene/Scene.h"

namespace maple
{
	auto EntityManager::create() -> Entity
	{
		return Entity(registry.create(), scene->getRegistry());
	}

	auto EntityManager::create(const std::string &name) -> Entity
	{
		auto e = registry.create();
		registry.emplace<NameComponent>(e, name);
		return Entity(e, scene->getRegistry());
	}

	auto EntityManager::clear() -> void
	{
		registry.each([&](auto entity) {
			registry.destroy(entity);
		});
		registry.clear();
	}

	auto EntityManager::removeAllChildren(entt::entity entity, bool root) -> void
	{
		auto hierarchyComponent = registry.try_get<Hierarchy>(entity);
		if (hierarchyComponent)
		{
			entt::entity child = hierarchyComponent->getFirst();
			while (child != entt::null)
			{
				auto hierarchyComponent = registry.try_get<Hierarchy>(child);
				auto next               = hierarchyComponent ? hierarchyComponent->getNext() : entt::null;
				removeAllChildren(child, false);
				child = next;
			}
		}
		if (!root)
			registry.destroy(entity);
	}

	auto EntityManager::removeEntity(entt::entity entity) -> void
	{
		auto hierarchyComponent = registry.try_get<Hierarchy>(entity);
		if (hierarchyComponent)
		{
			entt::entity child = hierarchyComponent->getFirst();
			while (child != entt::null)
			{
				auto hierarchyComponent = registry.try_get<Hierarchy>(child);
				auto next               = hierarchyComponent ? hierarchyComponent->getNext() : entt::null;
				removeEntity(child);
				child = next;
			}
		}
		registry.destroy(entity);
	}

	auto EntityManager::getEntityByName(const std::string &name) -> Entity
	{
		auto views = registry.view<NameComponent>();
		for (auto &view : views)
		{
			auto &comp = registry.get<NameComponent>(view);
			if (comp.name == name)
			{
				return {view, scene->getRegistry()};
			}
		}
		return {};
	}

};        // namespace maple
