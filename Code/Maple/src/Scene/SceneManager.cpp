
//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "SceneManager.h"
#include "Application.h"
#include "Engine/Camera.h"
#include "Engine/Core.h"
#include "Engine/Profiler.h"
#include "Entity/Entity.h"
#include "Others/StringUtils.h"
#include "Scene/Scene.h"
#include "Scripts/Mono/MonoSystem.h"
namespace maple
{
	auto SceneManager::switchScene(const std::string &name) -> void
	{
		switchingScenes = true;

		if (auto iter = allScenes.find(name); iter != allScenes.end())
		{
			switchingScenes = true;
			currentName     = name;
		}
		else
		{
			switchingScenes = false;
			LOGW("[{0} : {1}] - Unknown Scene : {2}", __FILE__, __LINE__, name);
		}
	}

	auto SceneManager::apply() -> void
	{
		PROFILE_FUNCTION();
		if (!switchingScenes)
		{
			if (currentScene != nullptr)
				return;
			if (allScenes.empty() || (allScenes.size() == 1 && hasPreviewScene()))
			{
				currentName = "default";
				addScene(currentName, new Scene(currentName));
			}
		}

		//switching to new scene
		if (currentScene != nullptr)        //clear before
		{
			currentScene->onClean();
		}

		currentScene = allScenes[currentName].get();

		currentScene->loadFrom();

		if (Application::get()->getEditorState() == EditorState::Play)
		{
			currentScene->onInit();
		}

		Application::get()->onSceneCreated(currentScene);

		switchingScenes = false;
	}

	auto SceneManager::addSceneFromFile(const std::string &filePath) -> void
	{
		auto name  = StringUtils::removeExtension(StringUtils::getFileName(filePath));
		auto scene = new Scene(name);
		scene->setPath(filePath);
		addScene(filePath, scene);
	}

	auto SceneManager::hasPreviewScene() -> bool
	{
		return getSceneByName("PreviewScene") != nullptr;
	}

	Scene *SceneManager::getSceneByName(const std::string &sceneName)
	{
		if (auto iter = allScenes.find(sceneName); iter != allScenes.end())
		{
			return iter->second.get();
		}
		return nullptr;
	}

	auto SceneManager::addScene(const std::string &name, Scene *scene) -> void
	{
		allScenes[name] = std::shared_ptr<Scene>(scene);

		using CameraQuery = ecs::Registry ::Modify<Camera>::To<ecs::Group>;

		CameraQuery query{
		    Application::getExecutePoint()->getRegistry(),
		    Application::getExecutePoint()->getGlobalEntity()};

		if (query.empty())
		{
			auto  entity = scene->createEntity("Main Camera");
			auto &camera = entity.addComponent<Camera>();
			camera.setFov(45.f);
			camera.setFar(100);
			camera.setNear(0.01);
			camera.setAspectRatio(4 / 3.f);
		}
	}
};        // namespace maple
