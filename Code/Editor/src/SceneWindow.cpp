//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "SceneWindow.h"

#include "RHI/GraphicsContext.h"
#include "RHI/Texture.h"

#include "Devices/Input.h"
#include "Editor.h"
#include "Engine/Camera.h"
#include "Event/WindowEvent.h"
#include "IconsMaterialDesignIcons.h"
#include "ImGui/ImGuiHelpers.h"
#include "Others/Console.h"
#include "Scene/Scene.h"

#include "imgui_internal.h"

#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>
#include <imGuIZMOquat.h>

namespace Maple
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
			bool click = drawToolBar();


			if (transform != nullptr)
			{
				ImVec2 offset            = {0.0f, 0.0f};
				auto   sceneViewSize     = ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin() - offset / 2.0f;        // - offset * 0.5f;
				auto   sceneViewPosition = ImGui::GetWindowPos() + offset;

				sceneViewSize.x -= static_cast<int>(sceneViewSize.x) % 2 != 0 ? 1.0f : 0.0f;
				sceneViewSize.y -= static_cast<int>(sceneViewSize.y) % 2 != 0 ? 1.0f : 0.0f;
				resize(static_cast<uint32_t>(sceneViewSize.x), static_cast<uint32_t>(sceneViewSize.y));

				auto quat = glm::inverse(transform->getWorldOrientation());

				ImGuiHelper::image(previewTexture.get(), {static_cast<uint32_t>(sceneViewSize.x), static_cast<uint32_t>(sceneViewSize.y)});

				auto   windowSize = ImGui::GetWindowSize();
				ImVec2 minBound   = sceneViewPosition;

				ImVec2 maxBound     = {minBound.x + windowSize.x, minBound.y + windowSize.y};
				bool   updateCamera = ImGui::IsMouseHoveringRect(minBound, maxBound);

				focused = ImGui::IsWindowFocused() && !ImGuizmo::IsUsing() && updateCamera;

				editor.setSceneActive(ImGui::IsWindowFocused() && !ImGuizmo::IsUsing() && updateCamera);

				ImGuizmo::SetRect(sceneViewPosition.x, sceneViewPosition.y, sceneViewSize.x, sceneViewSize.y);
				ImGui::GetWindowDrawList()->PushClipRect(sceneViewPosition, {sceneViewSize.x + sceneViewPosition.x, sceneViewSize.y + sceneViewPosition.y - 2.0f});

				if (editor.getEditorState() == EditorState::Preview && !showCamera && transform != nullptr)
				{
					const float *cameraViewPtr = glm::value_ptr(transform->getWorldMatrixInverse());

					if (camera->isOrthographic())
					{
						draw2DGrid(ImGui::GetWindowDrawList(),
						           {transform->getWorldPosition().x, transform->getWorldPosition().y}, sceneViewPosition, {sceneViewSize.x, sceneViewSize.y}, 1.0f, 1.5f);
					}

					ImGui::GetWindowDrawList()->PushClipRect(sceneViewPosition, {sceneViewSize.x + sceneViewPosition.x, sceneViewSize.y + sceneViewPosition.y - 2.0f});

					float viewManipulateRight = sceneViewPosition.x + sceneViewSize.x;
					float viewManipulateTop   = sceneViewPosition.y + 20;

					ImGui::SetItemAllowOverlap();
					ImGui::SetCursorPos({sceneViewSize.x - 96, viewManipulateTop});
					ImGui::gizmo3D("##gizmo1", quat, 96, imguiGizmo::sphereAtOrigin | imguiGizmo::modeFullAxes);

					editor.onImGuizmo();

					if (editor.isSceneActive() && !ImGuizmo::IsUsing() && ImGui::IsMouseReleased(ImGuiMouseButton_Left) && !click)
					{
						auto clickPos = Input::getInput()->getMousePosition() - glm::vec2(sceneViewPosition.x, sceneViewPosition.y);
						editor.clickObject(editor.getScreenRay(int32_t(clickPos.x), int32_t(clickPos.y), camera, int32_t(sceneViewSize.x), int32_t(sceneViewSize.y)));
					}
					drawGizmos(sceneViewSize.x, sceneViewSize.y, offset.x, offset.y, currentScene);
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

		if (previewTexture == nullptr)
		{
			previewTexture = Texture2D::create();
		}

		if (resized)
		{
			editor.getCamera()->setAspectRatio(width / (float) height);
			Application::get()->getGraphicsContext()->waitIdle();
			previewTexture->buildTexture(TextureFormat::RGBA8, width, height, false, false, false);
			Application::getRenderGraph()->setRenderTarget(previewTexture);
			Application::getRenderGraph()->onResize(width, height);
			Application::get()->getGraphicsContext()->waitIdle();
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

	auto SceneWindow::drawToolBar() -> bool
	{
		bool clicked = false;
		ImGui::Indent();
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.8f));
		bool  selected = false;
		auto &editor   = *static_cast<Editor *>(Application::get());
		{
			selected = editor.getImGuizmoOperation() == 4;
			if (selected)
				ImGui::PushStyleColor(ImGuiCol_Text, SelectedColor);
			ImGui::SameLine();
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
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
		ImGui::SameLine();

		{
			selected = editor.getImGuizmoOperation() == ImGuizmo::TRANSLATE;
			if (selected)
				ImGui::PushStyleColor(ImGuiCol_Text, SelectedColor);
			ImGui::SameLine();
			if (ImGui::Button(ICON_MDI_ARROW_ALL))
			{
				clicked = true;
				editor.setImGuizmoOperation(ImGuizmo::TRANSLATE);
			}

			if (selected)
				ImGui::PopStyleColor();
			ImGuiHelper::tooltip("Translate");
		}

		{
			selected = editor.getImGuizmoOperation() == ImGuizmo::ROTATE;
			if (selected)
				ImGui::PushStyleColor(ImGuiCol_Text, SelectedColor);

			ImGui::SameLine();
			if (ImGui::Button(ICON_MDI_ROTATE_ORBIT))
			{
				clicked = true;
				editor.setImGuizmoOperation(ImGuizmo::ROTATE);
			}

			if (selected)
				ImGui::PopStyleColor();
			ImGuiHelper::tooltip("Rotate");
		}

		{
			selected = editor.getImGuizmoOperation() == ImGuizmo::SCALE;
			if (selected)
				ImGui::PushStyleColor(ImGuiCol_Text, SelectedColor);

			ImGui::SameLine();
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

			ImGui::SameLine();
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

		bool ortho = camera->isOrthographic();

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
		ImGui::SameLine();

		selected = ortho;
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

		ImGui::PopStyleColor();
		ImGui::Unindent();

		return clicked;
	}

};        // namespace Maple
