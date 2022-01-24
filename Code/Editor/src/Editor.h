//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Application.h"
#include "EditorWindow.h"
#include "Engine/CameraController.h"
#include "FileSystem/File.h"
#include "Math/Ray.h"
#include "Scene/Component/Transform.h"
#include "UI/LoadingDialog.h"
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <memory>
namespace maple
{
	class EditorPlugin;
	class Texture2D;
	class SceneWindow;
	class TextureAtlas;
	class RenderManager;
	class Quad2D;

	class Editor final : public Application
	{
	  public:

		Editor(AppDelegate *appDelegate);
		auto init() -> void override;
		auto onImGui() -> void override;
		auto onUpdate(const Timestep &delta) -> void override;

		auto onRenderDebug() -> void override;

		auto setSelected(const entt::entity &selectedNode) -> void;
		auto setSelected(const std::string &selectResource) -> void;

		auto setCopiedEntity(const entt::entity &selectedNode, bool cut = false) -> void;

		auto getScreenRay(int32_t x, int32_t y, Camera *camera, int32_t width, int32_t height) -> Ray;

		template <class T>
		inline auto getEditorWindow()
		{
			return std::static_pointer_cast<T>(editorWindows[typeid(T).hash_code()]);
		}

		template <class T>
		auto addSubWindow() -> void;

		inline auto &getSelected() const
		{
			return selectedNode;
		}
		inline auto &getPrevSelected() const
		{
			return prevSelectedNode;
		}
		inline auto &getCopiedEntity() const
		{
			return copiedNode;
		}
		inline auto isCutCopyEntity() const
		{
			return cutCopyEntity;
		}
		inline auto &getComponentIconMap() const
		{
			return iconMap;
		}
		inline auto &getComponentIconMap()
		{
			return iconMap;
		}

		inline auto &getEditorCameraTransform()
		{
			return editorCameraTransform;
		}
		inline auto &getEditorCameraController()
		{
			return editorCameraController;
		}

		inline auto getImGuizmoOperation() const
		{
			return imGuizmoOperation;
		}
		inline auto setImGuizmoOperation(uint32_t imGuizmoOperation)
		{
			this->imGuizmoOperation = imGuizmoOperation;
		}

		inline auto setEditingResource(const std::string &selectResource)
		{
			this->editingResource = selectResource;
		}

		inline auto &getCamera()
		{
			return camera;
		}

		inline auto getTextureAtlas()
		{
			return textureAtlas;
		}

		inline auto &getSelectResource() const
		{
			return selectResource;
		}

		inline auto isLockInspector() const
		{
			return lockInspector;
		}

		inline auto &getEditingResource() const
		{
			return editingResource;
		}

		inline auto setLockInspector(bool lock)
		{
			return lockInspector = lock;
		}

		auto onImGuizmo() -> void;

		auto onSceneCreated(Scene *scene) -> void override;

		auto getIconFontIcon(const std::string &file) -> const char *;

		auto openFileInPreview(const std::string &file) -> void;
		auto openFileInEditor(const std::string &file) -> void;

		auto drawGrid() -> void;

		auto getIcon(FileType type) -> Quad2D *;
		auto processIcons() -> void;

		auto addPlugin(EditorPlugin *plugin) -> void;
		auto addFunctionalPlugin(const std::function<void(Editor *)> &callback) -> void;

		auto clickObject(const Ray &ray) -> void;
		auto focusCamera(const glm::vec3 &point, float distance, float speed = 1.0f) -> void;
		auto drawPlayButtons() -> void;

	  private:
		  
		auto drawMenu() -> void;
		auto beginDockSpace() -> void;
		auto endDockSpace() -> void;
		auto loadCachedScene() -> void;
		auto cacheScene() -> void;
		auto checkStencil(const entt::entity &selectedNode, bool enable) -> void;

		std::unordered_map<size_t, std::shared_ptr<EditorWindow>> editorWindows;

		entt::entity selectedNode     = entt::null;
		entt::entity prevSelectedNode = entt::null;
		entt::entity copiedNode       = entt::null;

		std::unordered_map<size_t, const char *> iconMap;

		uint32_t imGuizmoOperation = 4;

		bool cutCopyEntity  = false;
		bool showGizmos     = true;
		bool cameraSelected = false;
		bool lockInspector  = false;

		std::unique_ptr<Camera> camera;

		//need to be optimized. should use Atlats to cache.
		std::unordered_map<FileType, std::string>  cacheIcons;
		std::shared_ptr<TextureAtlas>              textureAtlas;
		std::vector<std::unique_ptr<EditorPlugin>> plugins;

		LoadingDialog          dialog;
		Transform              editorCameraTransform;
		EditorCameraController editorCameraController;

		std::string selectResource;
		std::string editingResource;

	  private:
		bool      transitioningCamera = false;
		glm::vec3 cameraDestination;
		glm::vec3 cameraStartPosition;
		float     cameraTransitionStartTime = 0.0f;
		float     cameraTransitionSpeed     = 0.0f;
	};

	template <class T>
	auto Editor::addSubWindow() -> void
	{
		editorWindows.emplace(typeid(T).hash_code(), std::make_shared<T>());
	}
};        // namespace maple
