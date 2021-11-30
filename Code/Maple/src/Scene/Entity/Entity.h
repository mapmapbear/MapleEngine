//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Others/Console.h"
#include "Scene/Component/Component.h"
#include "Scene/Scene.h"
#include "Scene/SceneGraph.h"

namespace Maple
{
	class MAPLE_EXPORT Entity
	{
	  public:
		Entity() = default;

		Entity(entt::entity handle, Scene *s) :
		    entityHandle(handle), scene(s)
		{
		}

		~Entity()
		{
		}

		template <typename T, typename... Args>
		inline T &addComponent(Args &&...args)
		{
#ifdef MAPLE_DEBUG
			if (hasComponent<T>())
				LOGW("Attempting to add extisting component ");
#endif
			T &t = scene->getRegistry().emplace<T>(entityHandle, std::forward<Args>(args)...);
			t.setEntity(entityHandle);
			return t;
		}

		template <typename T, typename... Args>
		inline T &getOrAddComponent(Args &&...args)
		{
			T &t = scene->getRegistry().get_or_emplace<T>(entityHandle, std::forward<Args>(args)...);
			t.setEntity(entityHandle);
			return t;
		}

		template <typename T, typename... Args>
		inline auto addOrReplaceComponent(Args &&...args)
		{
			T &t = scene->getRegistry().emplace_or_replace<T>(entityHandle, std::forward<Args>(args)...);
			t.setEntity(entityHandle);
		}

		template <typename T>
		inline T &getComponent()
		{
			return scene->getRegistry().get<T>(entityHandle);
		}

		template <typename T>
		inline T *tryGetComponent()
		{
			return scene->getRegistry().try_get<T>(entityHandle);
		}

		template <typename T>
		inline T *tryGetComponentFromParent()
		{
			auto t = scene->getRegistry().try_get<T>(entityHandle);
			if (t == nullptr)
			{
				auto parent = getParent();
				while (parent.valid() && t == nullptr)
				{
					t      = parent.tryGetComponent<T>();
					parent = parent.getParent();
				}
			}
			return t;
		}

		template <typename T>
		inline auto hasComponent() const -> bool
		{
			return scene->getRegistry().has<T>(entityHandle);
		}

		template <typename T>
		inline auto removeComponent()
		{
			return scene->getRegistry().remove<T>(entityHandle);
		}

		auto isActive() -> bool;
		auto setActive(bool active) -> void;
		auto setParent(const Entity &entity) -> void;
		auto findByPath(const std::string &path) -> Entity;
		auto getChildInChildren(const std::string &name) -> Entity;
		auto getParent() -> Entity;
		auto getChildren() -> std::vector<Entity>;
		auto removeAllChildren() -> void;

		auto isParent(const Entity &potentialParent) const -> bool;

		inline operator entt::entity() const
		{
			return entityHandle;
		}
		inline operator uint32_t() const
		{
			return (uint32_t) entityHandle;
		}
		inline operator bool() const
		{
			return entityHandle != entt::null && scene;
		}

		inline auto operator==(const Entity &other) const
		{
			return entityHandle == other.entityHandle && scene == other.scene;
		}
		inline auto operator!=(const Entity &other) const
		{
			return !(*this == other);
		}

		inline auto getHandle() const
		{
			return entityHandle;
		}
		inline auto setHandle(entt::entity en)
		{
			entityHandle = en;
		}
		inline auto getScene() const
		{
			return scene;
		}
		inline auto setScene(Scene *sc)
		{
			scene = sc;
		}

		auto destroy() -> void;
		auto valid() -> bool;

	  protected:
		entt::entity entityHandle = entt::null;
		Scene *      scene        = nullptr;
		friend class EntityManager;
	};

	template <typename TComponentChain>
	class AccessEntity : public Entity
	{
	  public:
		static constexpr auto ComponentTypes = TComponentChain::getComponentList();

		AccessEntity() = default;

		AccessEntity(entt::entity handle, Scene *s) :
		    entityHandle(handle), scene(s)
		{
		}

		AccessEntity(Entity entity)
		{
			this->scene        = entity.getScene();
			this->entityHandle = entity.getHandle();
		}

		~AccessEntity()
		{
		}

		template <typename Comp>
		inline auto hasComponent() const -> bool
		{
			constexpr auto chain = TComponentChain();
			constexpr auto index = chain.findByType<Comp>();

			if constexpr (index >= 0)
			{
				constexpr auto entry = chain.getEntry(index);
				return scene->getRegistry().has<Comp>(entityHandle);
			}
			return false;
		}

		template <typename Comp>
		inline decltype(auto) getComponent() const
		{
			constexpr auto chain = TComponentChain();
			constexpr auto Index = chain.findByType<Comp>();

			if constexpr (Index >= 0)
			{
				return get<Index::value>();
			}
			else
			{
				static_cast(false, "Comp is not in access type");
				return *static_cast<Comp *>(nullptr);
			}
		}

		template <int32_t index>
		inline decltype(auto) get() const
		{
			constexpr auto chain      = TComponentChain();
			constexpr auto entry      = chain.getEntry(std::integral_constant<int32_t, index>());
			constexpr auto returnType = entry.getReturnType();
			using RPtr                = typename decltype(returnType)::value *;
			return *const_cast<RPtr>(scene->getRegistry().try_get<typename decltype(entry)::Component>(entityHandle));
		}
	};
};        // namespace Maple

namespace std
{
	template <typename TComponentChain>
	struct tuple_size<Maple::AccessEntity<TComponentChain>>
	{
		static constexpr auto   chain = TComponentChain();
		static constexpr size_t value = chain.Size;
	};

	template <size_t i, typename TComponentChain>
	struct tuple_element<i, Maple::AccessEntity<TComponentChain>>
	{
		static constexpr auto chain      = TComponentChain();
		static constexpr auto entry      = chain.getEntry(std::integral_constant<int32_t, i>());
		static constexpr auto returnType = entry.getReturnType();
		using type                       = typename decltype(returnType)::value &;
	};
};        // namespace std
