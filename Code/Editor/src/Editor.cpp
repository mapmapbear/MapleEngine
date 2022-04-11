//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "Editor.h"
#include "Main.h"
#include <imgui.h>

#include <ImGuizmo.h>

#include <glm/gtc/type_ptr.hpp>
#include <imgui_internal.h>

#include "AssetsWindow.h"
#include "DisplayZeroWindow.h"
#include "HierarchyWindow.h"
#include "PreviewWindow.h"
#include "PropertiesWindow.h"
#include "VisualizeCacheWindow.h"
#include "RenderGraphWindow.h"
#include "SceneWindow.h"
#include "CurveWindow.h"
#include "Devices/Input.h"

#include "Engine/Camera.h"
#include "Engine/Renderer/GeometryRenderer.h"
#include "Engine/TextureAtlas.h"


#include "IconsMaterialDesignIcons.h"
#include "ImGui/ImGuiHelpers.h"
#include "2d/Sprite.h"

#include "Scene/Component/Component.h"
#include "Scene/Component/Light.h"
#include "Scene/Component/Transform.h"
#include "Scene/Component/BoundingBox.h"
#include "Scene/Entity/Entity.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Scene/Component/MeshRenderer.h"

#include "Physics/RigidBody.h"
#include "Physics/Collider.h"
#include "Physics/PhysicsSystem.h"
#include "Physics/PhysicsWorld.h"

#include "FileSystem/File.h"
#include "Loaders/Loader.h"

#include "Others/StringUtils.h"

#include "EditorPlugin.h"
#include "Plugin/PluginWindow.h"

#include "Math/BoundingBox.h"
#include "Math/MathUtils.h"
#include "Math/Ray.h"
#include "Scripts/Mono/MonoVirtualMachine.h"
#include "Scripts/Mono/MonoComponent.h"

#include "ImGui/ImNotification.h"
#include <ecs/ecs.h>

namespace maple
{
	Editor::Editor(AppDelegate *appDelegate) :
	    Application(appDelegate)
	{
		editor = true;
	}
#define addWindow(T) editorWindows.emplace(typeid(T).hash_code(), std::make_shared<T>())
	auto Editor::init() -> void
	{
		Application::init();
		//off-screen
		addWindow(SceneWindow);
		//addWindow(DisplayZeroWindow);
		addWindow(HierarchyWindow);
		addWindow(PropertiesWindow);
		addWindow(AssetsWindow);
		addWindow(VisualizeCacheWindow);
		//addWindow(PreviewWindow);
		addWindow(RenderGraphWindow);
		addWindow(CurveWindow);

		ImGuizmo::SetGizmoSizeClipSpace(0.25f);
		auto winSize = window->getWidth() / (float) window->getHeight();

		camera = std::make_unique<Camera>(
		    60, 0.1, 3000, winSize);
		editorCameraController.setCamera(camera.get());

		setEditorState(EditorState::Preview);

		if (File::fileExists("default.scene"))
		{
			sceneManager->addSceneFromFile("default.scene");
			sceneManager->switchScene("default.scene");
		}

		processIcons();


		ImNotification::makeNotification("tips", "Compiling C# Assembly...", ImNotification::Type::Info);

		MonoVirtualMachine::get()->compileAssembly([&](void *) {
			MonoVirtualMachine::get()->loadAssembly("./", "MapleLibrary.dll");
			MonoVirtualMachine::get()->loadAssembly("./", "MapleAssembly.dll");
			addWindow(PluginWindow);
			ImNotification::makeNotification("tips", "Compiling success", ImNotification::Type::Info);
		});
	}

	auto Editor::onImGui() -> void
	{
		drawMenu();
		dialog.onImGui();
		beginDockSpace();
		//drawPlayButtons();
		for (auto &win : editorWindows)
		{
			win.second->onImGui();
		}
		endDockSpace();
		Application::onImGui();
	}

	auto Editor::onUpdate(const Timestep &delta) -> void
	{
		if (auto previewScene = sceneManager->getSceneByName("PreviewScene"); previewScene != nullptr)
		{
			previewScene->onUpdate(delta);
		}
		Application::onUpdate(delta);

		auto currentScene = sceneManager->getCurrentScene();
		if (getEditorState() == EditorState::Preview)
		{

			/*	if (isSceneActive())
			{
				
			}*/

			for (auto &win : editorWindows)
			{
				if (win.second->isFocused())
				{
					win.second->handleInput(delta);
				}
			}

			if (!Input::getInput()->isMouseHeld(KeyCode::MouseKey::ButtonRight) && !ImGuizmo::IsUsing() && isSceneActive() && selectedNode != entt::null)
			{
				if (Input::getInput()->isKeyPressed(KeyCode::Id::Q))
				{
					setImGuizmoOperation(4);
				}

				if (Input::getInput()->isKeyPressed(KeyCode::Id::W))
				{
					setImGuizmoOperation(ImGuizmo::OPERATION::TRANSLATE);
				}

				if (Input::getInput()->isKeyPressed(KeyCode::Id::E))
				{
					setImGuizmoOperation(ImGuizmo::OPERATION::ROTATE);
				}

				if (Input::getInput()->isKeyPressed(KeyCode::Id::R))
				{
					setImGuizmoOperation(ImGuizmo::OPERATION::SCALE);
				}

				if (Input::getInput()->isKeyPressed(KeyCode::Id::T))
				{
					setImGuizmoOperation(ImGuizmo::OPERATION::BOUNDS);
				}
			}

			if ((Input::getInput()->isKeyHeld(KeyCode::Id::LeftControl) ||
			     Input::getInput()->isKeyHeld(KeyCode::Id::RightControl)) &&
			    Input::getInput()->isKeyPressed(KeyCode::Id::S))
			{
				serialize();
			}
			editorCameraTransform.setWorldMatrix(glm::mat4(1.f));
		}

		/*
		if (transitioningCamera)
		{
			if (cameraTransitionStartTime < 0.0f)
				cameraTransitionStartTime = delta.getMilliseconds();

			float focusProgress = std::min((delta.getMilliseconds() - cameraTransitionStartTime) / cameraTransitionSpeed, 1.f);
			auto newCameraPosition = MathUtils::lerp(cameraStartPosition,cameraDestination, focusProgress);
			editorCameraTransform.setLocalPosition(newCameraPosition);

			if (MathUtils::equals(editorCameraTransform.getLocalPosition(),cameraDestination))
				transitioningCamera = false;
		}
*/

		for (auto &plugin : plugins)
		{
			if (!plugin->isInited())
			{
				plugin->process(this);
				plugin->setInited(true);
			}
		}
	}

	auto Editor::onRenderDebug() -> void
	{
		if (selectedNode == entt::null)
		{
			return;
		}

		auto &registry = getExecutePoint()->getRegistry();
		if (cameraSelected || camera->isOrthographic())
		{
			auto view = registry.view<Camera, component::Transform>();
			for (auto v : view)
			{
				auto &[camera, trans] = registry.get<Camera, component::Transform>(v);
				GeometryRenderer::drawFrustum(camera.getFrustum(glm::inverse(trans.getWorldMatrix())));
			}
		}

		if (auto sprite = registry.try_get<component::Sprite>(selectedNode))
		{
			auto &transform = registry.get<component::Transform>(selectedNode);
			auto  pos       = transform.getWorldPosition() + glm::vec3(sprite->getQuad().getOffset(), 0);
			auto  w         = sprite->getQuad().getWidth();
			auto  h         = sprite->getQuad().getHeight();
			GeometryRenderer::drawRect(pos.x, pos.y, w, h);
		}

		if (auto sprite = registry.try_get<component::AnimatedSprite>(selectedNode))
		{
			auto &transform = registry.get<component::Transform>(selectedNode);
			auto  pos       = transform.getWorldPosition() + glm::vec3(sprite->getQuad().getOffset(), 0);
			auto  w         = sprite->getQuad().getWidth();
			auto  h         = sprite->getQuad().getHeight();
			GeometryRenderer::drawRect(pos.x, pos.y, w, h);
		}

		if (auto light = registry.try_get<component::Light>(selectedNode))
		{
			auto& transform = registry.get<component::Transform>(selectedNode);
			auto  pos = transform.getWorldPosition();
			GeometryRenderer::drawLight(light, transform.getWorldOrientation(), glm::vec4(glm::vec3(light->lightData.color), 0.2f));
		}

		if (auto render = registry.try_get<component::MeshRenderer>(selectedNode))
		{
			auto& transform = registry.get<component::Transform>(selectedNode);

			if (auto mesh = render->getMesh()) 
			{
				auto bb = mesh->getBoundingBox()->transform(transform.getWorldMatrix());
				GeometryRenderer::drawBox(bb, {1,1,1,1});
			}
		}

		if (auto collider = registry.try_get<physics::component::Collider>(selectedNode))
		{
			auto& transform = registry.get<component::Transform>(selectedNode);
			auto pos = transform.getWorldPosition();

			if (collider->type == physics::ColliderType::BoxCollider) 
			{
				GeometryRenderer::drawBox(collider->box, { 0,1,0,1 });
			}

			else if (collider->type == physics::ColliderType::SphereCollider) 
			{
				GeometryRenderer::drawSphere(collider->radius, pos, { 0,1,0,1 });
			}
		}

	}

	auto Editor::setSelected(const entt::entity &node) -> void
	{
		prevSelectedNode = selectedNode;
		checkStencil(selectedNode, false);
		checkStencil(node, true);
		selectedNode   = node;

		cameraSelected = node != entt::null && getExecutePoint()->getRegistry().try_get<Camera>(selectedNode) != nullptr;
	}

	auto Editor::setSelected(const std::string &selectResource) -> void
	{
		LOGI("Selected File is {0}", selectResource);
		this->selectResource = selectResource;
	}

	auto Editor::setCopiedEntity(const entt::entity &selectedNode, bool cut) -> void
	{
		copiedNode = selectedNode;
	}

	auto Editor::onImGuizmo() -> void
	{
		auto view = editorCameraTransform.getWorldMatrixInverse();
		auto proj = camera->getProjectionMatrix();

		if (selectedNode == entt::null || imGuizmoOperation == ImGuizmo::SELECT)
			return;

		if (showGizmos)
		{
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetOrthographic(camera->isOrthographic());

			auto & registry = getExecutePoint()->getRegistry();
			auto  transform = registry.try_get <component::Transform>(selectedNode);
			auto  rigidBody = registry.try_get <physics::component::RigidBody>(selectedNode);
			auto  collider = registry.try_get <physics::component::Collider>(selectedNode);

			if (transform != nullptr)
			{
				auto model = transform->getWorldMatrix();

				float delta[16];
				float box[6] = {};

				ImGuizmo::Manipulate(
					glm::value_ptr(view),
					glm::value_ptr(proj),
					static_cast<ImGuizmo::OPERATION>(imGuizmoOperation),
					ImGuizmo::LOCAL,
					glm::value_ptr(model),
					delta,
					nullptr
				);
			
				if (ImGuizmo::IsUsing())
				{
					if (static_cast<ImGuizmo::OPERATION>(imGuizmoOperation) == ImGuizmo::OPERATION::SCALE)
					{
						auto mat = glm::make_mat4(delta);
						transform->setLocalScale(component::Transform::getScaleFromMatrix(mat));
					}
					else
					{
						auto mat = glm::make_mat4(delta) * transform->getLocalMatrix();
						transform->setLocalTransform(mat);
						//TOOD
					}
				}
			}
		}
	}

	auto Editor::drawPlayButtons() -> void
	{
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.2f, 0.7f, 0.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 100);

			if (getEditorState() == EditorState::Next)
				setEditorState(EditorState::Paused);

			bool selected;
			{
				selected = getEditorState() == EditorState::Play;
				if (selected)
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.9f, 1.0f));

				if (ImGui::Button(selected ? ICON_MDI_STOP : ICON_MDI_PLAY))
				{
					setEditorState(selected ? EditorState::Preview : EditorState::Play);

					setSelected((entt::entity) entt::null);

					if (selected)
						loadCachedScene();
					else
					{
						cacheScene();
						sceneManager->getCurrentScene()->onInit();
					}
				}

				ImGuiHelper::tooltip("Play");
				if (selected)
					ImGui::PopStyleColor();
			}

			ImGui::SameLine();

			{
				selected = getEditorState() == EditorState::Paused;
				if (selected)
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.9f, 1.0f));

				if (ImGui::Button(ICON_MDI_PAUSE))
					setEditorState(selected ? EditorState::Play : EditorState::Paused);

				ImGuiHelper::tooltip("Pause");

				if (selected)
					ImGui::PopStyleColor();
			}

			ImGui::SameLine();

			{
				selected = getEditorState() == EditorState::Next;
				if (selected)
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.56f, 0.9f, 1.0f));

				if (ImGui::Button(ICON_MDI_STEP_FORWARD))
					setEditorState(EditorState::Next);

				ImGuiHelper::tooltip("Next");

				if (selected)
					ImGui::PopStyleColor();
			}

			ImGui::PopStyleVar();
			ImGui::PopStyleColor();
		}
		//ImGui::PopStyleVar();
		//ImGui::End();
	}

	auto Editor::drawMenu() -> void
	{
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Exit"))
				{
				}

				if (ImGui::MenuItem("Open File"))
				{
					File::openFile([](int32_t, const std::string &file) {
						LOGI("{0}", file);
					});
				}

				if (ImGui::MenuItem("Save (Ctrl + S)"))
				{
					serialize();
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Scripts"))
			{
				if (ImGui::MenuItem("Compile C# Assembly"))
				{
					dialog.show("Compiling ....");
					MonoVirtualMachine::get()->compileAssembly([&](void *) {
						dialog.close();
						MonoVirtualMachine::get()->loadAssembly("./", "MapleAssembly.dll");
					});
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Window"))
			{
#define OPEN_WINDOW(T,WindowName)					\
				if (ImGui::MenuItem(WindowName))	\
				{									\
					getEditorWindow<T>()->setActive(true); \
				}

				OPEN_WINDOW(CurveWindow, "Curve Editor");

				OPEN_WINDOW(RenderGraphWindow, "Render Graph");

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}
	}

	auto Editor::beginDockSpace() -> void
	{
		static bool               p_open                    = true;
		static bool               opt_fullscreen_persistant = true;
		static ImGuiDockNodeFlags opt_flags                 = ImGuiDockNodeFlags_NoWindowMenuButton | ImGuiDockNodeFlags_NoCloseButton;
		bool                      opt_fullscreen            = opt_fullscreen_persistant;

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			ImGuiViewport *viewport = ImGui::GetMainViewport();

			auto pos     = viewport->Pos;
			auto size    = viewport->Size;
			bool menuBar = true;
			if (menuBar)
			{
				const float infoBarSize = ImGui::GetFrameHeight();
				pos.y += infoBarSize;
				size.y -= infoBarSize;
			}

			ImGui::SetNextWindowPos(pos);
			ImGui::SetNextWindowSize(size);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}

		if (opt_flags & ImGuiDockNodeFlags_DockSpace)
			window_flags |= ImGuiWindowFlags_NoBackground;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("MyDockspace", &p_open, window_flags);
		ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		ImGuiID DockspaceID = ImGui::GetID("MyDockspace");

		if (!ImGui::DockBuilderGetNode(DockspaceID))
		{
			ImGui::DockBuilderRemoveNode(DockspaceID);        // Clear out existing layout
			ImGui::DockBuilderAddNode(DockspaceID);           // Add empty node
			ImGui::DockBuilderSetNodeSize(DockspaceID, ImGui::GetIO().DisplaySize);

			ImGuiID dock_main_id = DockspaceID;
			ImGuiID DockBottom   = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.3f, nullptr, &dock_main_id);
			ImGuiID DockLeft     = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.2f, nullptr, &dock_main_id);
			ImGuiID DockRight    = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.20f, nullptr, &dock_main_id);

			ImGuiID DockLeftChild         = ImGui::DockBuilderSplitNode(DockLeft, ImGuiDir_Down, 0.875f, nullptr, &DockLeft);
			ImGuiID DockRightChild        = ImGui::DockBuilderSplitNode(DockRight, ImGuiDir_Down, 0.875f, nullptr, &DockRight);
			ImGuiID DockingLeftDownChild  = ImGui::DockBuilderSplitNode(DockLeftChild, ImGuiDir_Down, 0.06f, nullptr, &DockLeftChild);
			ImGuiID DockingRightDownChild = ImGui::DockBuilderSplitNode(DockRightChild, ImGuiDir_Down, 0.06f, nullptr, &DockRightChild);

			ImGuiID DockBottomChild         = ImGui::DockBuilderSplitNode(DockBottom, ImGuiDir_Down, 0.2f, nullptr, &DockBottom);
			ImGuiID DockingBottomLeftChild  = ImGui::DockBuilderSplitNode(DockBottomChild, ImGuiDir_Left, 0.5f, nullptr, &DockBottomChild);
			ImGuiID DockingBottomRightChild = ImGui::DockBuilderSplitNode(DockBottomChild, ImGuiDir_Right, 0.5f, nullptr, &DockBottomChild);

			ImGuiID DockMiddle = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.8f, nullptr, &dock_main_id);

			ImGui::DockBuilderDockWindow(SceneWindow::STATIC_NAME, DockMiddle);
			ImGui::DockBuilderDockWindow(RenderGraphWindow::STATIC_NAME, DockMiddle);
			ImGui::DockBuilderDockWindow("Display", DockMiddle);

			ImGui::DockBuilderDockWindow(PropertiesWindow::STATIC_NAME, DockRight);
			ImGui::DockBuilderDockWindow(VisualizeCacheWindow::STATIC_NAME, DockRight);
			ImGui::DockBuilderDockWindow("Console", DockingBottomLeftChild);
			ImGui::DockBuilderDockWindow(AssetsWindow::STATIC_NAME, DockingBottomRightChild);
			ImGui::DockBuilderDockWindow(HierarchyWindow::STATIC_NAME, DockLeft);
			ImGui::DockBuilderDockWindow(PreviewWindow::STATIC_NAME, DockingRightDownChild);

			ImGui::DockBuilderFinish(DockspaceID);
		}

		// Dockspace
		ImGuiIO &io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGui::DockSpace(DockspaceID, ImVec2(0.0f, 0.0f), opt_flags);
		}
	}

	auto Editor::endDockSpace() -> void
	{
		ImGui::End();
	}

	auto Editor::loadCachedScene() -> void
	{
	}

	auto Editor::cacheScene() -> void
	{
	}

	auto Editor::checkStencil(const entt::entity &selectedNode, bool enable) -> void
	{
		auto& registry = getExecutePoint()->getRegistry();

		if (selectedNode == entt::null)
		{
			return;
		}

		if (enable)
		{
			registry.emplace_or_replace<component::StencilComponent>(selectedNode);
		}
		else
		{
			registry.remove_if_exists<component::StencilComponent>(selectedNode);
		}
	}

	auto Editor::onSceneCreated(Scene *scene) -> void
	{
		Application::onSceneCreated(scene);

		for (auto &wind : editorWindows)
		{
			wind.second->onSceneCreated(scene);
		}
	}

	auto Editor::getIconFontIcon(const std::string &filePath) -> const char *
	{
		if (StringUtils::isLuaFile(filePath))
		{
			return ICON_MDI_LANGUAGE_LUA;
		}
		else if (StringUtils::isCSharpFile(filePath))
		{
			return ICON_MDI_LANGUAGE_CSHARP;
		}
		else if (StringUtils::isTextFile(filePath))
		{
			return ICON_MDI_FILE_XML;
		}
		else if (StringUtils::isModelFile(filePath))
		{
			return ICON_MDI_SHAPE;
		}
		else if (StringUtils::isAudioFile(filePath))
		{
			return ICON_MDI_FILE_MUSIC;
		}
		else if (StringUtils::isTextureFile(filePath))
		{
			return ICON_MDI_FILE_IMAGE;
		}
		else if (StringUtils::isSceneFile(filePath))
		{
			return ICON_MDI_LEAF_MAPLE;
		}

		return ICON_MDI_FILE;
	}

	auto Editor::openFileInPreview(const std::string &filePath) -> void
	{
		auto scene = sceneManager->getSceneByName("PreviewScene");
		if (scene && filePath != "")
		{
			auto meshRoot = getExecutePoint()->getEntityByName("MeshRoot");
			LOGI("Open File in Preview Window {0}", filePath);
			meshRoot.removeAllChildren();

			auto fileType = File::getFileType(filePath);
			switch (fileType)
			{
				case maple::FileType::Normal:
					break;
				case maple::FileType::Folder:
					break;
				case maple::FileType::Texture:
					break;
				case maple::FileType::Model:
				case maple::FileType::FBX:
				case maple::FileType::OBJ: {
					if (meshRoot)
					{
						auto  name        = StringUtils::getFileNameWithoutExtension(filePath);
						auto  modelEntity = scene->createEntity(name);
						auto &model       = modelEntity.addComponent<component::Model>(filePath);
						if (model.resource->getMeshes().size() == 1)
						{
							modelEntity.addComponent<component::MeshRenderer>(model.resource->getMeshes().begin()->second);
						}
						else
						{
							for (auto &mesh : model.resource->getMeshes())
							{
								auto child = scene->createEntity(mesh.first);
								child.addComponent<component::MeshRenderer>(mesh.second);
								child.setParent(modelEntity);
							}
						}
						model.type = component::PrimitiveType::File;
						modelEntity.setParent(meshRoot);
					}
				}
				break;
				case maple::FileType::Text:
					break;
				case maple::FileType::Script:
					break;
				case maple::FileType::Dll:
					break;
				case maple::FileType::Scene: {
					sceneManager->addSceneFromFile(filePath);
					sceneManager->switchScene(filePath);
				}
				break;
				case maple::FileType::MP3:
					break;
				case maple::FileType::OGG:
					break;
				case maple::FileType::AAC:
					break;
				case maple::FileType::WAV:
					break;
				case maple::FileType::TTF:
					break;
				case maple::FileType::C_SHARP:
					break;
				case maple::FileType::Shader:
					break;
				case maple::FileType::Material: {
					auto entity                       = scene->createEntity("Sphere");
					entity.addComponent<component::Model>().type = component::PrimitiveType::Sphere;
					auto  mesh                        = Mesh::createSphere();
					auto &meshRender                  = entity.addComponent<component::MeshRenderer>(mesh);
					mesh->setMaterial(Material::create(filePath));
					entity.setParent(meshRoot);
				}
				break;
				case maple::FileType::Length:
					break;
				default:
					break;
			}
		}
	}

	auto Editor::openFileInEditor(const std::string &filePath) -> void
	{
		auto ext = StringUtils::getExtension(filePath);

		if (StringUtils::isTextFile(filePath))
		{
			LOGW("OpenFile file : {0} did not implement", filePath);
		}
		else if ( StringUtils::isModelFile(filePath) || loaderFactory->getSupportExtensions().count(ext) >= 1 )
		{
			selectedNode = sceneManager->getCurrentScene()->addMesh(filePath).getHandle();
		}
		else if (StringUtils::isAudioFile(filePath))
		{
			LOGW("OpenFile file : {0} did not implement", filePath);
		}
		else if (StringUtils::isSceneFile(filePath))
		{
			sceneManager->addSceneFromFile(filePath);
			sceneManager->switchScene(filePath);
		}
		else if (StringUtils::isTextureFile(filePath))
		{
			LOGW("OpenFile file : {0} did not implement", filePath);
		}
	}

	auto Editor::drawGrid() -> void
	{
		GeometryRenderer::drawLine(
		    {-5000, 0, 0}, {5000, 0, 0}, {1, 0, 0, 1.f});        //x
		GeometryRenderer::drawLine(
		    {0, -5000, 0}, {0, 5000, 0}, {0, 1, 0, 1.f});        //y
		GeometryRenderer::drawLine(
		    {0, 0, -5000}, {0, 0, 5000}, {0, 0, 1, 1.f});        //z
	}

	auto Editor::getIcon(FileType type) -> Quad2D *
	{
		if (auto iter = cacheIcons.find(type); iter != cacheIcons.end())
		{
			return textureAtlas->addSprite(iter->second);
		}

		return textureAtlas->addSprite(cacheIcons[FileType::Normal]);
	}

	auto Editor::processIcons() -> void
	{
#ifdef MAPLE_VULKAN
		constexpr bool flipY = false;
#else
		constexpr bool flipY = true;
#endif        // MAPLE_VULKAN
		textureAtlas = std::make_shared<TextureAtlas>(4096, 4096);
		int32_t i    = 0;
		for (auto &str : EditorInBuildIcon)
		{
			if (str != "")
			{
				cacheIcons[static_cast<FileType>(i++)] = str;
				textureAtlas->addSprite(str, flipY);
			}
		}
	}

	auto Editor::addPlugin(EditorPlugin *plugin) -> void
	{
		plugins.emplace_back(std::unique_ptr<EditorPlugin>(plugin));
	}

	auto Editor::addFunctionalPlugin(const std::function<void(Editor *)> &callback) -> void
	{
		plugins.emplace_back(std::make_unique<FunctionalPlugin>(callback));
	}

	auto Editor::clickObject(const Ray& ray) -> void
	{
		auto& registry = getExecutePoint()->getRegistry();

		float        closestDist = INFINITY;
		entt::entity closestEntity = entt::null;
		auto frustum = camera->getFrustum(editorCameraTransform.getWorldMatrixInverse());
		auto calculateClosest = [&](Mesh* mesh, component::Transform& trans, entt::entity entity)
		{
			if (mesh != nullptr)
			{
				auto& worldTransform = trans.getWorldMatrix();

				if (mesh->getBoundingBox() != nullptr)
				{
					auto  bbCopy = mesh->getBoundingBox()->transform(worldTransform);

					if (frustum.isInside(bbCopy)) 
					{
						float dist = ray.hit(bbCopy);
						if (dist < INFINITY && dist < closestDist && dist != 0.0f)
						{
							closestDist = dist;
							closestEntity = entity;
						}
					}
				}
			}
		};

		using Query = ecs::Chain
			::Write<component::MeshRenderer>
			::Write<component::Transform>
			::To<ecs::Query>;

		Query meshQuery(registry, entt::null);

		for (auto entity : meshQuery)
		{
			auto [mesh, trans] = meshQuery.convert(entity);
			calculateClosest(mesh.getMesh().get(), trans, entity);
		}

		using SkinnedQuery = ecs::Chain
			::Write<component::SkinnedMeshRenderer>
			::Write<component::Transform>
			::To<ecs::Query>;

		SkinnedQuery skinedQuery(registry, entt::null);

		for (auto entity : skinedQuery)
		{
			auto [mesh, trans] = skinedQuery.convert(entity);
			calculateClosest(mesh.getMesh().get(), trans, entity);
		}

		if (closestEntity == entt::null)
		{
			setImGuizmoOperation(ImGuizmo::OPERATION::SELECT);
		}

		static auto lastClick = timer.current();

		if (selectedNode != entt::null && selectedNode == closestEntity)
		{
			if (timer.elapsed(timer.current(), lastClick) / 1000000.f < 1.f)
			{
				auto &trans = registry.get<component::Transform>(selectedNode);
				auto model = registry.try_get<component::MeshRenderer>(selectedNode);
				auto skinned = registry.try_get<component::SkinnedMeshRenderer>(selectedNode);

				if (model) 
				{
					if (auto mesh = model->getMesh(); mesh != nullptr)
					{
						auto bb = mesh->getBoundingBox()->transform(trans.getWorldMatrix());
						focusCamera(trans.getWorldPosition(), glm::length(bb.max - bb.min));
					}
				}
				if (skinned) 
				{
					if (auto mesh = skinned->getMesh(); mesh != nullptr)
					{
						auto bb = mesh->getBoundingBox()->transform(trans.getWorldMatrix());
						focusCamera(trans.getWorldPosition(), glm::length(bb.max - bb.min));
					}
				}
			}
			else
			{
				closestEntity = entt::null;
			}

			lastClick = timer.current();
			setSelected(selectedNode);
			setImGuizmoOperation(ImGuizmo::OPERATION::TRANSLATE);
			return;
		}
		auto spriteGroup = registry.group<component::Sprite>(entt::get<component::Transform>);

		for (auto entity : spriteGroup)
		{
			const auto &[sprite, trans]     = spriteGroup.get<component::Sprite, component::Transform>(entity);
			auto &           worldTransform = trans.getWorldMatrix();
			const glm::vec2 &min            = sprite.getQuad().getOffset();
			glm::vec2        max            = min + glm::vec2{sprite.getQuad().getWidth(), sprite.getQuad().getHeight()};
			BoundingBox      bb{Rect2D(min, max)};
			bb.transform(trans.getWorldMatrix());

			float dist = ray.hit(bb);

			if (dist < INFINITY && dist < closestDist)
			{
				closestDist   = dist;
				closestEntity = entity;
				setImGuizmoOperation(ImGuizmo::OPERATION::TRANSLATE);
			}
		}

		auto animSpriteGroup = registry.group<component::AnimatedSprite>(entt::get<component::Transform>);

		for (auto entity : animSpriteGroup)
		{
			const auto &[sprite, trans]     = animSpriteGroup.get<component::AnimatedSprite, component::Transform>(entity);
			auto &           worldTransform = trans.getWorldMatrix();
			const glm::vec2 &min            = sprite.getQuad().getOffset();
			glm::vec2        max            = min + glm::vec2{sprite.getQuad().getWidth(), sprite.getQuad().getHeight()};
			BoundingBox      bb{Rect2D(min, max)};
			bb = bb.transform(trans.getWorldMatrix());

			float dist = ray.hit(bb);

			if (dist < INFINITY && dist < closestDist)
			{
				closestDist   = dist;
				closestEntity = entity;
				setImGuizmoOperation(ImGuizmo::OPERATION::TRANSLATE);
			}
		}

		if (selectedNode != entt::null && selectedNode == closestEntity)
		{
			auto &  trans  = registry.get<component::Transform>(selectedNode);
			auto sprite = registry.try_get<component::Sprite>(selectedNode);
			if (sprite == nullptr)
			{
				sprite = registry.try_get<component::AnimatedSprite>(selectedNode);
			}
			if (sprite != nullptr)
			{
				const glm::vec2 &min = sprite->getQuad().getOffset();
				const glm::vec2  max = min + glm::vec2{sprite->getQuad().getWidth(), sprite->getQuad().getHeight()};
				BoundingBox      bb(Rect2D(min, max));
				bb = bb.transform(trans.getWorldMatrix());
				focusCamera(trans.getWorldPosition(), glm::length(bb.max - bb.min));
			}
		}

		//if (closestEntity != entt::null)
		//selectedNode = closestEntity;
		setSelected(closestEntity);
	}

	auto Editor::focusCamera(const glm::vec3 &point, float distance, float speed /*= 1.0f*/) -> void
	{
		if (camera->isOrthographic())
		{
			editorCameraTransform.setLocalPosition(point + glm::vec3{0, 0, 0.1});
		}
		else
		{
			cameraDestination = point + editorCameraTransform.getForwardDirection() * distance;

			editorCameraTransform.setLocalPosition(cameraDestination);
		}
	}

	auto Editor::getScreenRay(int32_t x, int32_t y, Camera *camera, int32_t width, int32_t height) -> Ray
	{
		if (!camera)
			return {};

		float screenX = (float) x / (float) width;
		float screenY = (float) y / (float) height;

		bool flipY = true;

		return camera->sendRay(screenX, screenY, glm::inverse(editorCameraTransform.getWorldMatrix()), flipY);
	}

}        // namespace maple

#if !defined(EDITOR_STATIC)
maple::Application *createApplication()
{
	return new maple::Editor(new maple::DefaultDelegate());
}
#endif
