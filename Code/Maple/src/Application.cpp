//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "Application.h"
#include "Engine/Camera.h"
#include "Engine/Profiler.h"
#include "Engine/Terrain.h"
#include "Engine/Timestep.h"
#include "Engine/Renderer/Renderer2D.h"

#include "FileSystem/MeshLoader.h"
#include "Others/Console.h"
#include "Window/WindowWin.h"

#include "Scripts/Lua/LuaSystem.h"
#include "Scripts/Mono/MonoSystem.h"

#include "Devices/Input.h"
#include "ImGui/ImGuiSystem.h"
#include "ImGui/ImNotification.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Scene/System/AccessSystem.h"
#include "Scripts/Mono/MonoVirtualMachine.h"

#include <ecs/ecs.h>
#include <imgui.h>

//maple::Application* app;

namespace maple
{
	Application::Application(AppDelegate *app)
	{
		appDelegate     = std::shared_ptr<AppDelegate>(app);
		window          = NativeWindow::create(WindowInitData{1280, 720, false, "Maple-Engine"});
		renderDevice    = RenderDevice::create();
		graphicsContext = GraphicsContext::create();

		cache         = std::make_shared<Cache>();
		sceneManager  = std::make_unique<SceneManager>();
		threadPool    = std::make_unique<ThreadPool>(4);
		texturePool   = std::make_unique<TexturePool>();
		luaVm         = std::make_unique<LuaVirtualMachine>();
		monoVm        = std::make_shared<MonoVirtualMachine>();
		renderGraph   = std::make_shared<RenderGraph>();
		systemManager = std::make_unique<SystemManager>();
	
	}

	auto Application::init() -> void
	{
		PROFILE_FUNCTION();
		Input::create();
		window->init();

		graphicsContext->init();
		renderDevice->init();

		timer.start();
		luaVm->init();
		monoVm->init();
		renderGraph->init(window->getWidth(), window->getHeight());
		appDelegate->onInit();

		systemManager->addSystem<LuaSystem>()->onInit();
		systemManager->addSystem<MonoSystem>()->onInit();

		auto access = systemManager->addSystem<AccessSystem>();

		imGuiManager = systemManager->addSystem<ImGuiSystem>(false);
		imGuiManager->onInit();
	}

	auto Application::start() -> int32_t
	{
		double lastFrameTime = 0;
		init();

		while (1)
		{
			PROFILE_FRAMEMARKER();
			Input::getInput()->resetPressed();
			Timestep timestep = timer.stop() / 1000000.f;
			imGuiManager->newFrame(timestep);
			{
				sceneManager->apply();
				executeAll();
				onUpdate(timestep);
				onRender();

				imGuiManager->onRender(sceneManager->getCurrentScene());
				renderDevice->present();        //present all data
				window->swapBuffers();
			}
			graphicsContext->clearUnused();
			lastFrameTime += timestep;
			if (lastFrameTime - secondTimer > 1.0f)        //tick later
			{
				secondTimer += 1.0f;
				frames  = 0;
				updates = 0;
			}
		}
		appDelegate->onDestory();
		return 0;
	}

	//update all things
	auto Application::onUpdate(const Timestep &delta) -> void
	{
		PROFILE_FUNCTION();
		auto scene = sceneManager->getCurrentScene();
		scene->onUpdate(delta);
		onImGui();
		ImNotification::onImGui();
		systemManager->onUpdate(delta, scene);
		window->onUpdate();
		dispatcher.dispatchEvents();
		renderGraph->onUpdate(delta, scene);
	}

	auto Application::onRender() -> void
	{
		PROFILE_FUNCTION();
		renderDevice->begin();

		renderGraph->beginPreviewScene(sceneManager->getSceneByName("PreviewScene"));
		renderGraph->onRenderPreview();

		renderGraph->beginScene(sceneManager->getCurrentScene());
		renderGraph->onRender();

		onRenderDebug();
	}

	auto Application::beginScene() -> void
	{
	}

	auto Application::onImGui() -> void
	{
		PROFILE_FUNCTION();
	}

	auto Application::setSceneActive(bool active) -> void
	{
		sceneActive = active;
	}

	auto Application::onSceneCreated(Scene *scene) -> void
	{
	}

	auto Application::onWindowResized(uint32_t w, uint32_t h) -> void
	{
		PROFILE_FUNCTION();
		if (w == 0 || h == 0)
		{
			return;
		}

		graphicsContext->waitIdle();
		renderDevice->onResize(w, h);
		imGuiManager->onResize(w, h);
		renderGraph->onResize(w, h);
		graphicsContext->waitIdle();
	}

	auto Application::onRenderDebug() -> void
	{
	}

	auto Application::serialize() -> void
	{
		PROFILE_FUNCTION();
		if (sceneManager->getCurrentScene() != nullptr)
		{
			sceneManager->getCurrentScene()->saveTo();
			window->setTitle(sceneManager->getCurrentScene()->getName());
		}
	}

	auto Application::postOnMainThread(const std::function<bool()> &mainCallback) -> std::future<bool>
	{
		PROFILE_FUNCTION();
		std::promise<bool> promise;
		std::future<bool>  future = promise.get_future();

		std::lock_guard<std::mutex> locker(executeMutex);
		executeQueue.emplace(std::move(promise), mainCallback);
		return future;
	}

	auto Application::executeAll() -> void
	{
		PROFILE_FUNCTION();
		std::pair<std::promise<bool>, std::function<bool(void)>> func;
		for (;;)
		{
			{
				std::lock_guard<std::mutex> lock(executeMutex);
				if (executeQueue.empty())
					break;
				func = std::move(executeQueue.front());
				executeQueue.pop();
			}
			if (func.second)
			{
				func.first.set_value(func.second());
			}
		}
	}

	maple::Application *Application::app = nullptr;

};        // namespace maple
