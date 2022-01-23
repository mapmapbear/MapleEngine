//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "SceneWindow.h"

#include "RHI/GraphicsContext.h"
#include "RHI/Texture.h"

#include "Devices/Input.h"
#include "Editor.h"
#include "Engine/Camera.h"
#include "Engine/GBuffer.h"
#include "Event/WindowEvent.h"
#include "IconsMaterialDesignIcons.h"
#include "ImGui/ImGuiHelpers.h"
#include "Others/Console.h"
#include "Scene/Scene.h"

#include "imgui_internal.h"

#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>
#include <imGuIZMOquat.h>

namespace maple
{
	const ImVec4 SelectedColor(0.28f, 0.56f, 0.9f, 1.0f);
	SceneWindow::SceneWindow()
	{
		title = "Scene";
	}

	auto SceneWindow::onImGui() -> void
	{
		auto &editor = *static_cast<Editor *>(Application::get());

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		auto flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
		ImGui::SetNextWindowBgAlpha(0.0f);

		auto currentScene = Application::get()->getSceneManager()->getCurrentScene();
		if (ImGui::Begin(title.c_str(), &active, flags))
		{
			Camera *   camera    = nullptr;
			Transform *transform = nullptr;

			bool gameView = false;

			if (editor.getEditorState() == EditorState::Preview && !showCamera)
			{
				camera    = editor.getCamera().get();
				transform = &editor.getEditorCameraTransform();
				currentScene->setOverrideCamera(camera);
				currentScene->setOverrideTransform(transform);
			}
			else
			{
				gameView = true;
				currentScene->setOverrideCamera(nullptr);
				currentScene->setOverrideTransform(nullptr);

				auto &registry   = currentScene->getRegistry();
				auto  cameraView = registry.view<Camera, Transform>();
				if (!cameraView.empty())
				{
					camera    = &registry.get<Camera>(cameraView.front());
					transform = &registry.get<Transform>(cameraView.front());
				}
			}
			bool click = false;
			//drawToolBar();
			glm::quat quat = glm::identity<glm::quat>();

			if (transform != nullptr)
			{
				quat = glm::inverse(transform->getWorldOrientation());
			}

			drawToolbarOverlay(quat);

			if (transform != nullptr)
			{
				ImVec2 vMin          = ImGui::GetWindowContentRegionMin();
				ImVec2 vMax          = ImGui::GetWindowContentRegionMax();
				auto   sceneViewSize = vMax - vMin;        // - offset * 0.5f;

				sceneViewSize.x -= static_cast<int>(sceneViewSize.x) % 2 != 0 ? 1.0f : 0.0f;
				sceneViewSize.y -= static_cast<int>(sceneViewSize.y) % 2 != 0 ? 1.0f : 0.0f;
				resize(static_cast<uint32_t>(sceneViewSize.x), static_cast<uint32_t>(sceneViewSize.y));
				ImGuiHelper::image(previewTexture.get(), {static_cast<uint32_t>(sceneViewSize.x), static_cast<uint32_t>(sceneViewSize.y)});

				bool updateCamera = false;

				{
					vMin.x += ImGui::GetWindowPos().x;
					vMin.y += ImGui::GetWindowPos().y;
					vMax.x += ImGui::GetWindowPos().x;
					vMax.y += ImGui::GetWindowPos().y;
					updateCamera = ImGui::IsMouseHoveringRect(vMin, vMax);
				}

				focused = ImGui::IsWindowFocused() && !ImGuizmo::IsUsing() && updateCamera;

				editor.setSceneActive(ImGui::IsWindowFocused() && !ImGuizmo::IsUsing() && updateCamera);

				ImGuizmo::SetRect(vMin.x, vMin.y, sceneViewSize.x, sceneViewSize.y);

				if (editor.getEditorState() == EditorState::Preview && !showCamera && transform != nullptr)
				{
					const float *cameraViewPtr = glm::value_ptr(transform->getWorldMatrixInverse());

					if (camera->isOrthographic())
					{
						draw2DGrid(ImGui::GetWindowDrawList(),
						           {transform->getWorldPosition().x, transform->getWorldPosition().y}, vMin, sceneViewSize, 1.0f, 1.5f);
					}

					editor.onImGuizmo();

					if (editor.isSceneActive() && !ImGuizmo::IsUsing() && ImGui::IsMouseReleased(ImGuiMouseButton_Left) && !click)
					{
						auto clickPos = Input::getInput()->getMousePosition() - glm::vec2(vMin.x, vMin.y);
						editor.clickObject(editor.getScreenRay(int32_t(clickPos.x), int32_t(clickPos.y), camera, int32_t(sceneViewSize.x), int32_t(sceneViewSize.y)));
					}
				}

				if (ImGui::BeginDragDropTarget())
				{
					auto data = ImGui::AcceptDragDropPayload("AssetFile");
					if (data)
					{
						std::string file = (char *) data->Data;
						LOGV("Receive file from assets window : {0}", file);
						editor.openFileInEditor(file);
					}
					ImGui::EndDragDropTarget();
				}
			}
			ImGui::End();
		}
		ImGui::PopStyleVar();
	}

	auto SceneWindow::resize(uint32_t width, uint32_t height) -> void
	{
		bool  resized = false;
		auto &editor  = *static_cast<Editor *>(Application::get());
		if (this->width != width || this->height != height)
		{
			resized      = true;
			this->width  = width;
			this->height = height;
		}

		if (resized)
		{
			if (previewTexture == nullptr)
			{
				previewTexture = Texture2D::create();
				previewTexture->setName("PreviewTexture");
			}
			editor.getCamera()->setAspectRatio(this->width / (float) this->height);
			//editor.getGraphicsContext()->waitIdle();
			previewTexture->buildTexture(TextureFormat::RGBA8, this->width, this->height, false, false, false);
			editor.getRenderGraph()->setRenderTarget(previewTexture);
			editor.getRenderGraph()->onResize(this->width, this->height);
			//editor.getGraphicsContext()->waitIdle();
		}
	}

	auto SceneWindow::handleInput(float dt) -> void
	{
		auto &     editor   = *static_cast<Editor *>(Application::get());
		const auto mousePos = Input::getInput()->getMousePosition();
		editor.getEditorCameraController().handleMouse(editor.getEditorCameraTransform(), dt, mousePos.x, mousePos.y);
		editor.getEditorCameraController().handleKeyboard(editor.getEditorCameraTransform(), dt);
	}

	auto SceneWindow::drawGizmos(float width, float height, float xpos, float ypos, Scene *scene) -> void
	{
	}

	auto SceneWindow::drawToolbarOverlay(glm::quat &quat) -> bool
	{
		constexpr float  DISTANCE    = 30.0f;
		constexpr int    corner      = 0;
		ImGuiIO &        io          = ImGui::GetIO();
		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
		if (corner != -1)
		{
			windowFlags |= ImGuiWindowFlags_NoMove;
			const ImGuiViewport *viewport       = ImGui::GetWindowViewport();
			const ImVec2         workAreaPos    = ImGui::GetCurrentWindow()->Pos;        // Instead of using viewport->Pos we use GetWorkPos() to avoid menu bars, if any!
			const ImVec2         workAreaSize   = ImGui::GetCurrentWindow()->Size;
			const ImVec2         windowPos      = ImVec2((corner & 1) ? (workAreaPos.x + workAreaSize.x - DISTANCE / 2) : (workAreaPos.x + DISTANCE / 2), (corner & 2) ? (workAreaPos.y + workAreaSize.y - DISTANCE) : (workAreaPos.y + DISTANCE));
			const ImVec2         windowPosPivot = ImVec2((corner & 1) ? 1.0f : 0.0f, (corner & 2) ? 1.0f : 0.0f);
			ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, windowPosPivot);
			ImGui::SetNextWindowViewport(viewport->ID);
		}
		ImGui::SetNextWindowBgAlpha(0.35f);        // Transparent background
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 35.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {5, 4.5});
		bool  selected = false;
		bool  clicked  = false;
		auto &editor   = *static_cast<Editor *>(Application::get());

		if (ImGui::Begin("Example: Simple overlay", &showOverlay, windowFlags))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 100);
			{
				selected = editor.getImGuizmoOperation() == 4;
				if (selected)
					ImGui::PushStyleColor(ImGuiCol_Text, SelectedColor);
				if (ImGui::Button(ICON_MDI_CURSOR_DEFAULT))
				{
					editor.setImGuizmoOperation(4);
					clicked = true;
				}

				if (selected)
					ImGui::PopStyleColor();
				ImGuiHelper::tooltip("Select");
			}
			ImGui::SameLine();

			if (!collapsed)
			{
				{
					selected = editor.getImGuizmoOperation() == ImGuizmo::TRANSLATE;
					if (selected)
						ImGui::PushStyleColor(ImGuiCol_Text, SelectedColor);
					if (ImGui::Button(ICON_MDI_ARROW_ALL))
					{
						clicked = true;
						editor.setImGuizmoOperation(ImGuizmo::TRANSLATE);
					}

					if (selected)
						ImGui::PopStyleColor();
					ImGuiHelper::tooltip("Translate");
				}
				ImGui::SameLine();

				{
					selected = editor.getImGuizmoOperation() == ImGuizmo::ROTATE;
					if (selected)
						ImGui::PushStyleColor(ImGuiCol_Text, SelectedColor);

					if (ImGui::Button(ICON_MDI_ROTATE_ORBIT))
					{
						clicked = true;
						editor.setImGuizmoOperation(ImGuizmo::ROTATE);
					}

					if (selected)
						ImGui::PopStyleColor();
					ImGuiHelper::tooltip("Rotate");
				}

				ImGui::SameLine();
				{
					selected = editor.getImGuizmoOperation() == ImGuizmo::SCALE;
					if (selected)
						ImGui::PushStyleColor(ImGuiCol_Text, SelectedColor);
					if (ImGui::Button(ICON_MDI_ARROW_EXPAND_ALL))
					{
						clicked = true;
						editor.setImGuizmoOperation(ImGuizmo::SCALE);
					}

					if (selected)
						ImGui::PopStyleColor();
					ImGuiHelper::tooltip("Scale");
				}

				ImGui::SameLine();
				ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
				ImGui::SameLine();

				{
					selected = editor.getImGuizmoOperation() == ImGuizmo::BOUNDS;
					if (selected)
						ImGui::PushStyleColor(ImGuiCol_Text, SelectedColor);

					if (ImGui::Button(ICON_MDI_BORDER_NONE))
					{
						clicked = true;
						editor.setImGuizmoOperation(ImGuizmo::BOUNDS);
					}

					if (selected)
						ImGui::PopStyleColor();
					ImGuiHelper::tooltip("Bounds");
				}

				ImGui::SameLine();
				ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
				ImGui::SameLine();
				auto &camera = editor.getCamera();
				bool  ortho  = camera->isOrthographic();
				{
					selected = !ortho;
					if (selected)
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0.4, 0.4, 1.f));
					if (ImGui::Button(ICON_MDI_AXIS_ARROW " 3D"))
					{
						clicked = true;
						if (ortho)
						{
							camera->setOrthographic(false);
							editor.getEditorCameraController().setTwoDMode(false);
						}
					}
					if (selected)
						ImGui::PopStyleColor();
					selected = ortho;
				}

				ImGui::SameLine();

				{
					if (selected)
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0.4, 0.4, 1.f));
					if (ImGui::Button(ICON_MDI_ANGLE_RIGHT "2D"))
					{
						clicked = true;
						if (!ortho)
						{
							camera->setOrthographic(true);
							editor.getEditorCameraController().setTwoDMode(true);
							editor.getEditorCameraTransform().setLocalOrientation({0, 0, 0});
						}
					}
					if (selected)
						ImGui::PopStyleColor();
				}

				ImGui::SameLine();

				if (ImGui::Button(ICON_MDI_ARROW_LEFT))
				{
					collapsed = true;
				}
			}
			else
			{
				ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
				ImGui::SameLine();
				if (ImGui::Button(ICON_MDI_ARROW_RIGHT))
				{
					collapsed = false;
				}
			}
			ImGui::PopStyleVar();
		}
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();
		ImGui::End();

		constexpr int32_t corner2   = 1;
		constexpr float   DISTANCE2 = 20.0f;

		windowFlags |= ImGuiWindowFlags_NoMove;
		windowFlags |= ImGuiWindowFlags_NoBackground;

		const ImGuiViewport *viewport       = ImGui::GetWindowViewport();
		const ImVec2         workAreaPos    = ImGui::GetCurrentWindow()->Pos;        // Instead of using viewport->Pos we use GetWorkPos() to avoid menu bars, if any!
		const ImVec2         workAreaSize   = ImGui::GetCurrentWindow()->Size;
		const ImVec2         windowPos      = ImVec2((corner2 & 1) ? (workAreaPos.x + workAreaSize.x - DISTANCE2 / 2) : (workAreaPos.x + DISTANCE2 / 2), (corner2 & 2) ? (workAreaPos.y + workAreaSize.y - DISTANCE2) : (workAreaPos.y + DISTANCE2));
		const ImVec2         windowPosPivot = ImVec2((corner2 & 1) ? 1.0f : 0.0f, (corner2 & 2) ? 1.0f : 0.0f);
		ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, windowPosPivot);
		ImGui::SetNextWindowViewport(viewport->ID);
		if (ImGui::Begin("Controls", &showOverlay, windowFlags))
		{
			ImGui::gizmo3D("##gizmo1", quat, 96, imguiGizmo::sphereAtOrigin | imguiGizmo::modeFullAxes);
		}
		ImGui::End();
		return clicked;
	}

	auto SceneWindow::draw2DGrid(ImDrawList *drawList, const ImVec2 &cameraPos, const ImVec2 &windowPos, const ImVec2 &canvasSize, const float factor, const float thickness) -> void
	{
		static const auto graduation = 10;
		float             GRID_SZ    = canvasSize.y * 0.5f / factor;
		const ImVec2 &    offset     = {
            canvasSize.x * 0.5f - cameraPos.x * GRID_SZ, canvasSize.y * 0.5f + cameraPos.y * GRID_SZ};

		ImU32 GRID_COLOR    = IM_COL32(200, 200, 200, 40);
		float gridThickness = 1.0f;

		const auto &gridColor       = GRID_COLOR;
		auto        smallGraduation = GRID_SZ / graduation;
		const auto &smallGridColor  = IM_COL32(100, 100, 100, smallGraduation);

		for (float x = windowPos.x; x < canvasSize.x + GRID_SZ; x += GRID_SZ)
		{
			auto localX = floorf(x + fmodf(offset.x, GRID_SZ));
			drawList->AddLine(
			    ImVec2{localX, 0.0f} + windowPos, ImVec2{localX, canvasSize.y} + windowPos, gridColor, gridThickness);

			if (smallGraduation > 5.0f)
			{
				for (int i = 1; i < graduation; ++i)
				{
					const auto graduation = floorf(localX + smallGraduation * i);
					drawList->AddLine(ImVec2{graduation, 0.0f} + windowPos,
					                  ImVec2{graduation, canvasSize.y} + windowPos,
					                  smallGridColor,
					                  1.0f);
				}
			}
		}

		for (float y = windowPos.y; y < canvasSize.y + GRID_SZ; y += GRID_SZ)
		{
			auto localY = floorf(y + fmodf(offset.y, GRID_SZ));
			drawList->AddLine(
			    ImVec2{0.0f, localY} + windowPos, ImVec2{canvasSize.x, localY} + windowPos, gridColor, gridThickness);

			if (smallGraduation > 5.0f)
			{
				for (int i = 1; i < graduation; ++i)
				{
					const auto graduation = floorf(localY + smallGraduation * i);
					drawList->AddLine(ImVec2{0.0f, graduation} + windowPos,
					                  ImVec2{canvasSize.x, graduation} + windowPos,
					                  smallGridColor,
					                  1.0f);
				}
			}
		}
	}

};        // namespace maple
