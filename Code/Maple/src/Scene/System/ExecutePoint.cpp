//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "ExecutePoint.h"
#include "Scene/Component/Component.h"
#include "Scene/Entity/Entity.h"

namespace maple
{
	auto ExecutePoint::clear() -> void
	{
		registry.each([&](auto entity) {
			registry.destroy(entity);
		});
		registry.clear();
	}

	auto ExecutePoint::removeAllChildren(entt::entity entity, bool root) -> void
	{
		auto hierarchyComponent = registry.try_get<component::Hierarchy>(entity);
		if (hierarchyComponent)
		{
			entt::entity child = hierarchyComponent->first;
			while (child != entt::null)
			{
				auto hierarchyComponent = registry.try_get<component::Hierarchy>(child);
				auto next               = hierarchyComponent ? hierarchyComponent->next : entt::null;
				removeAllChildren(child, false);
				child = next;
			}
		}
		if (!root)
			registry.destroy(entity);
	}

	auto ExecutePoint::removeEntity(entt::entity entity) -> void
	{
		auto hierarchyComponent = registry.try_get<component::Hierarchy>(entity);
		if (hierarchyComponent)
		{
			entt::entity child = hierarchyComponent->first;
			while (child != entt::null)
			{
				auto hierarchyComponent = registry.try_get<component::Hierarchy>(child);
				auto next               = hierarchyComponent ? hierarchyComponent->next : entt::null;
				removeEntity(child);
				child = next;
			}
		}
		registry.destroy(entity);
	}

	auto ExecutePoint::create() -> Entity
	{
		return Entity(registry.create(), getRegistry());
	}

	auto ExecutePoint::create(const std::string &name) -> Entity
	{
		auto e = registry.create();
		registry.emplace<component::NameComponent>(e, name);
		return Entity(e, getRegistry());
	}

	auto ExecutePoint::getEntityByName(const std::string &name) -> Entity
	{
		auto views = registry.view<component::NameComponent>();
		for (auto &view : views)
		{
			auto &comp = registry.get<component::NameComponent>(view);
			if (comp.name == name)
			{
				return {view, getRegistry()};
			}
		}
		return {};
	}
};        // namespace maple
