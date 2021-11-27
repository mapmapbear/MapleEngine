//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Engine/Core.h"
#include <entt/entt.hpp>
#include <functional>
#include <memory>
#include <string>

namespace Maple
{
	class EntityManager;
	class Entity;
	class SceneGraph;
	class Camera;
	class Transform;

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
		inline auto setOverrideTransform(Transform *overrideTransform)
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

		auto getCamera() -> std::pair<Camera *, Transform *>;

		inline auto &getSettings() const
		{
			return settings;
		}

		inline auto &getSettings()
		{
			return settings;
		}

		struct Settings
		{
			bool render2D       = true;
			bool render3D       = true;
			bool renderDebug    = true;
			bool renderSkybox   = true;
			bool renderShadow   = true;
			bool deferredRender = true;
		};

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

	  protected:
		auto updateCameraController(float dt) -> void;
		auto copyComponents(const Entity &from, const Entity &to) -> void;

		std::shared_ptr<SceneGraph>    sceneGraph;
		std::shared_ptr<EntityManager> entityManager;
		std::string                    name;
		std::string                    filePath;

		uint32_t width  = 0;
		uint32_t height = 0;

		bool                              inited            = false;
		Camera *                          overrideCamera    = nullptr;
		Transform *                       overrideTransform = nullptr;
		std::function<void(Scene *scene)> initCallback;

		int32_t version;

		bool     dirty          = false;
		bool     useSceneCamera = false;
		Settings settings;

		std::function<void(Entity)> onEntityAdd;
	};
};        // namespace Maple
