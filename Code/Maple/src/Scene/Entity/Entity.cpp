//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "Entity.h"
#include "Others/StringUtils.h"

namespace maple
{
	auto Entity::isActive() -> bool
	{
		if (hasComponent<ActiveComponent>())
			return registry->get<ActiveComponent>(entityHandle).active;

		return true;
	}

	auto Entity::setActive(bool isActive) -> void
	{
		getOrAddComponent<ActiveComponent>().active = isActive;
	}

	auto Entity::setParent(const Entity &entity) -> void
	{
		bool acceptable         = false;
		auto hierarchyComponent = tryGetComponent<Hierarchy>();
		if (hierarchyComponent != nullptr)
		{
			acceptable = entity.entityHandle != entityHandle && (!entity.isParent(*this)) && (hierarchyComponent->getParent() != entityHandle);
		}
		else
			acceptable = entity.entityHandle != entityHandle;

		if (!acceptable)
			return;

		if (hierarchyComponent)
			Hierarchy::reparent(entityHandle, entity.entityHandle, *registry, *hierarchyComponent);
		else
		{
			registry->emplace<Hierarchy>(entityHandle, entity.entityHandle);
		}
	}

	Entity Entity::findByPath(const std::string &path)
	{
		if (path == "")
		{
			return {};
		}
		Entity ent      = {};
		auto   layers   = StringUtils::split(path, "/");
		auto   children = getChildren();

		for (int32_t i = 0; i < layers.size(); ++i)
		{
			bool findChild = false;

			if (layers[i] == "..")
			{
				ent = getParent();
			}
			else
			{
				for (auto entt : children)
				{
					auto &nameComp = entt.getComponent<NameComponent>();
					if (layers[i] == nameComp.name)
					{
						ent      = entt;
						children = ent.getChildren();
						break;
					}
				}
			}
		}
		return ent;
	}

	Entity Entity::getChildInChildren(const std::string &name)
	{
		auto children = getChildren();
		for (auto entt : children)
		{
			auto &nameComp = entt.getComponent<NameComponent>();
			if (name == nameComp.name)
			{
				return entt;
			}
			auto ret = entt.getChildInChildren(name);
			if (ret.valid())
			{
				return ret;
			}
		}
		return {};
	}

	Entity Entity::getParent()
	{
		auto hierarchyComp = tryGetComponent<Hierarchy>();
		if (hierarchyComp)
			return Entity(hierarchyComp->getParent(), *registry);
		else
			return {};
	}

	std::vector<Entity> Entity::getChildren()
	{
		std::vector<Entity> children;
		auto                hierarchyComponent = tryGetComponent<Hierarchy>();
		if (hierarchyComponent)
		{
			entt::entity child = hierarchyComponent->getFirst();
			while (child != entt::null && registry->valid(child))
			{
				children.emplace_back(child, *registry);
				hierarchyComponent = registry->try_get<Hierarchy>(child);
				if (hierarchyComponent)
					child = hierarchyComponent->getNext();
			}
		}
		return children;
	}

	auto Entity::removeAllChildren() -> void
	{
		auto hierarchyComponent = registry->try_get<Hierarchy>(entityHandle);
		if (hierarchyComponent)
		{
			entt::entity child = hierarchyComponent->getFirst();
			while (child != entt::null)
			{
				auto   hierarchyComponent = registry->try_get<Hierarchy>(child);
				auto   next               = hierarchyComponent ? hierarchyComponent->getNext() : entt::null;
				Entity ent                = {child, *registry};
				ent.removeAllChildren();
				ent.destroy();
				child = next;
			}
		}
	}

	bool Entity::isParent(const Entity &potentialParent) const
	{
		auto nodeHierarchyComponent = registry->try_get<Hierarchy>(entityHandle);
		if (nodeHierarchyComponent)
		{
			auto parent = nodeHierarchyComponent->getParent();
			while (parent != entt::null)
			{
				if (parent == potentialParent.entityHandle)
				{
					return true;
				}
				else
				{
					nodeHierarchyComponent = registry->try_get<Hierarchy>(parent);
					parent                 = nodeHierarchyComponent ? nodeHierarchyComponent->getParent() : entt::null;
				}
			}
		}
		return false;
	}

	auto Entity::destroy() -> void
	{
		registry->destroy(entityHandle);
	}

	auto Entity::valid() -> bool
	{
		return registry && registry->valid(entityHandle);
	}

};        // namespace maple
