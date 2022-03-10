//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Engine/Core.h"
#include "Scene/Entity/Entity.h"
#include "Math/BoundingBox.h"
#include <functional>
#include <memory>
#include <string>

namespace maple
{
	class EntityManager;
	class Entity;
	class SceneGraph;
	class Camera;
	namespace component 
	{
		class Transform;
	}

	class MAPLE_EXPORT Scene
	{
	  public:
		Scene(const std::string &name);
		virtual ~Scene() = default;

		virtual auto onInit() -> void;
		virtual auto onClean() -> void;
		virtual auto onUpdate(float dt) -> void;
		virtual auto saveTo(const std::string &filePath = "", bool binary = false) -> void;
		virtual auto loadFrom() -> void;

		inline auto setOverrideCamera(Camera *overrideCamera)
		{
			this->overrideCamera = overrideCamera;
		}
		inline auto setOverrideTransform(component::Transform *overrideTransform)
		{
			this->overrideTransform = overrideTransform;
		}
		inline auto setUseSceneCamera(bool useSceneCamera)
		{
			this->useSceneCamera = useSceneCamera;
		}

		inline auto &getEntityManager()
		{
			return entityManager;
		}
		inline auto &getName() const
		{
			return name;
		};
		inline auto &getPath() const
		{
			return filePath;
		};

		inline auto setPath(const std::string &path)
		{
			this->filePath = path;
		}

		inline auto setName(const std::string &name)
		{
			this->name = name;
		}
		inline auto setInitCallback(const std::function<void(Scene *scene)> &call)
		{
			initCallback = call;
		}
		inline auto setOnEntityAddListener(const std::function<void(Entity)> &call)
		{
			onEntityAdd = call;
		}

		auto setSize(uint32_t w, uint32_t h) -> void;
		auto getRegistry() -> entt::registry &;

		auto createEntity() -> Entity;
		auto createEntity(const std::string &name) -> Entity;

		auto duplicateEntity(const Entity &entity, const Entity &parent) -> void;
		auto duplicateEntity(const Entity &entity) -> void;

		auto getCamera() -> std::pair<Camera *, component::Transform *>;

		auto removeAllChildren(entt::entity entity) -> void;

		template <typename Archive>
		auto save(Archive &archive) const -> void
		{
			archive(1, name);
		}

		template <typename Archive>
		auto load(Archive &archive) -> void
		{
			archive(version, name);
		}

		template <typename... Components>
		inline auto addGlobalComponent()
		{
			(getRegistry().emplace<Components>(globalEntity.getHandle());...);
		}

		template <typename Component, typename... Args>
		inline auto& getGlobalComponent(Args &&...args)
		{
			return globalEntity.template getOrAddComponent<Component>(std::forward<Args>(args)...);
		}

		inline auto getGlobalEntity() const 
		{
			return globalEntity;
		}

		inline auto& getBoundingBox() { if (boxDirty) calculateBoundingBox();  return sceneBox; }

		auto calculateBoundingBox() -> void;
		auto onMeshRenderCreated() -> void;
	  protected:
		auto updateCameraController(float dt) -> void;
		auto copyComponents(const Entity &from, const Entity &to) -> void;

		std::shared_ptr<SceneGraph>    sceneGraph;
		std::shared_ptr<EntityManager> entityManager;
		std::string                    name;
		std::string                    filePath;

		Entity globalEntity;

		uint32_t width  = 0;
		uint32_t height = 0;

		bool                              inited            = false;
		Camera *                          overrideCamera    = nullptr;
		component::Transform *                       overrideTransform = nullptr;
		std::function<void(Scene *scene)> initCallback;

		int32_t version;

		bool     dirty          = false;
		bool     useSceneCamera = false;

		std::function<void(Entity)> onEntityAdd;

		BoundingBox sceneBox;
		bool boxDirty = false;
	};
};        // namespace maple
