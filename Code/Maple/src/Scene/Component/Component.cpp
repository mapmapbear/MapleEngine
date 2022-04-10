//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "Component.h"
#include "RHI/Texture.h"
#include "Scene/Entity/Entity.h"
#include "Scene/SceneManager.h"
#include "Scene/Scene.h"

#include "Application.h"
#include "Scene/System/ExecutePoint.h"

namespace maple
{
	namespace component 
	{
		Hierarchy::Hierarchy(entt::entity p) :
			parent(p)
		{
			first = entt::null;
			next = entt::null;
			prev = entt::null;
		}

		Hierarchy::Hierarchy()
		{
		}

		auto Hierarchy::compare(const entt::registry& registry, entt::entity rhs) const -> bool
		{
			if (rhs == entt::null || rhs == parent || rhs == prev)
			{
				return true;
			}

			if (parent == entt::null)
			{
				return false;
			}
			else
			{
				auto& thisParent = registry.get<Hierarchy>(parent);
				auto& rhsParent = registry.get<Hierarchy>(rhs).parent;
				if (thisParent.compare(registry, parent))
					return true;
			}
			return false;
		}

		auto Hierarchy::reset() -> void
		{
			parent = entt::null;
			first = entt::null;
			next = entt::null;
			prev = entt::null;
		}

		auto Hierarchy::onConstruct(entt::registry& registry, entt::entity entity) -> void
		{
			auto& hierarchy = registry.get<Hierarchy>(entity);
			if (hierarchy.parent != entt::null)
			{
				auto& parentHierarchy = registry.get_or_emplace<Hierarchy>(hierarchy.parent);

				if (parentHierarchy.first == entt::null)
				{
					parentHierarchy.first = entity;
				}
				else
				{
					auto prevEnt = parentHierarchy.first;
					auto currentHierarchy = registry.try_get<Hierarchy>(prevEnt);

					while (currentHierarchy != nullptr && currentHierarchy->next != entt::null)
					{
						prevEnt = currentHierarchy->next;
						currentHierarchy = registry.try_get<Hierarchy>(prevEnt);
					}
					currentHierarchy->next = entity;
					hierarchy.prev = prevEnt;
				}
			}
		}

		auto Hierarchy::onDestroy(entt::registry& registry, entt::entity entity) -> void
		{
			auto& hierarchy = registry.get<Hierarchy>(entity);
			if (hierarchy.prev == entt::null || !registry.valid(hierarchy.prev))
			{
				if (hierarchy.parent != entt::null && registry.valid(hierarchy.parent))
				{
					auto parentHierarchy = registry.try_get<Hierarchy>(hierarchy.parent);
					if (parentHierarchy != nullptr)
					{
						parentHierarchy->first = hierarchy.next;
						if (hierarchy.next != entt::null)
						{
							auto nextHierarchy = registry.try_get<Hierarchy>(hierarchy.next);
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
				auto prevHierarchy = registry.try_get<Hierarchy>(hierarchy.prev);
				if (prevHierarchy != nullptr)
				{
					prevHierarchy->next = hierarchy.next;
				}
				if (hierarchy.next != entt::null)
				{
					auto nextHierarchy = registry.try_get<Hierarchy>(hierarchy.next);
					if (nextHierarchy != nullptr)
					{
						nextHierarchy->prev = hierarchy.prev;
					}
				}
			}
		}

		auto Hierarchy::onUpdate(entt::registry& registry, entt::entity entity) -> void
		{
			auto& hierarchy = registry.get<Hierarchy>(entity);
			// if is the first child
			if (hierarchy.prev == entt::null)
			{
				if (hierarchy.parent != entt::null)
				{
					auto parent_hierarchy = registry.try_get<Hierarchy>(hierarchy.parent);
					if (parent_hierarchy != nullptr)
					{
						parent_hierarchy->first = hierarchy.next;
						if (hierarchy.next != entt::null)
						{
							auto next_hierarchy = registry.try_get<Hierarchy>(hierarchy.next);
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
				auto prevHierarchy = registry.try_get<Hierarchy>(hierarchy.prev);
				if (prevHierarchy != nullptr)
				{
					prevHierarchy->next = hierarchy.next;
				}
				if (hierarchy.next != entt::null)
				{
					auto next_hierarchy = registry.try_get<Hierarchy>(hierarchy.next);
					if (next_hierarchy != nullptr)
					{
						next_hierarchy->prev = hierarchy.prev;
					}
				}
			}
		}

		auto Hierarchy::reparent(entt::entity ent, entt::entity parent, entt::registry& registry, Hierarchy& hierarchy) -> void
		{
			Hierarchy::onDestroy(registry, ent);

			hierarchy.parent = entt::null;
			hierarchy.next = entt::null;
			hierarchy.prev = entt::null;

			if (parent != entt::null)
			{
				hierarchy.parent = parent;
				Hierarchy::onConstruct(registry, ent);
			}
		}

		auto Component::getEntity() -> maple::Entity
		{
			return { entity, Application::getExecutePoint()->getRegistry() };
		}

		auto Component::setEntity(entt::entity entity) -> void
		{
			this->entity = entity;
		}

		Environment::Environment()
		{
		}

		Environment::Environment(const std::string& filePath)
		{
			init(filePath);
		}

		auto Environment::init(const std::string& filePath) -> void
		{
			this->filePath = filePath;
			if (filePath != "")
			{
				TextureLoadOptions options(false, true, true);
				TextureParameters  parameters(TextureFormat::RGBA32, TextureWrap::ClampToEdge);
				equirectangularMap = Texture2D::create(filePath, filePath, parameters, options);
				width = equirectangularMap->getWidth();
				height = equirectangularMap->getHeight();
				numMips = equirectangularMap->getMipMapLevels();
				irradianceMap = TextureCube::create(IrradianceMapSize, TextureFormat::RGBA32, 0);
				prefilteredEnvironment = TextureCube::create(PrefilterMapSize, TextureFormat::RGBA32, 7);
			}
		}
	}
}        // namespace maple
