
//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "PreviewWindow.h"
#include "Devices/Input.h"
#include "Engine/Camera.h"
#include "Engine/CameraController.h"
#include "ImGui/ImGuiHelpers.h"
#include "RHI/Texture.h"
#include "Scene/Component/Light.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Component/Transform.h"
#include "Scene/Entity/Entity.h"
#include "Scene/Scene.h"

#include "Application.h"
#include <imGuIZMOquat.h>

#include <ecs/ComponentChain.h>
#include "Scene/System/ExecutePoint.h"

namespace maple
{
	PreviewWindow::PreviewWindow()
	{
		scene = new Scene("PreviewScene");
		Application::getSceneManager()->addScene("PreviewScene", scene);

		auto entity = scene->createEntity("Light");
		entity.addComponent<component::Light>();

		auto &transform = entity.getComponent<component::Transform>();
		transform.setLocalOrientation(glm::radians(glm::vec3(45.0, 15.0, 45.0)));

		auto controller = scene->createEntity("CameraController");
		controller.addComponent<component::EditorCameraController>();

		auto meshRoot = scene->createEntity("MeshRoot");
		meshRoot.addComponent<component::Transform>();

		scene->setUseSceneCamera(true);
		scene->getCamera().second->setLocalPosition({0, 0, 3});
	}

	PreviewWindow::~PreviewWindow()
	{
	}

	auto PreviewWindow::onImGui() -> void
	{
		auto flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
		if (ImGui::Begin(STATIC_NAME, &active, flags))
		{
			auto   windowSize   = ImGui::GetWindowSize();
			ImVec2 minBound     = ImGui::GetWindowPos();
			ImVec2 maxBound     = {minBound.x + windowSize.x, minBound.y + windowSize.y};
			bool   updateCamera = ImGui::IsMouseHoveringRect(minBound, maxBound);
			setFocused(ImGui::IsWindowFocused() && updateCamera);

			Application::getRenderGraph()->setPreviewFocused(isFocused());
			if (!focused)
			{
				previousCurserPos = {};
			}

			auto sceneViewSize = ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin();
			if (sceneViewSize.y > 0 && sceneViewSize.x)
			{
				resize(static_cast<uint32_t>(sceneViewSize.x), static_cast<uint32_t>(sceneViewSize.y));
				ImGuiHelper::image(renderTexture.get(), { static_cast<uint32_t>(sceneViewSize.x), static_cast<uint32_t>(sceneViewSize.y) });
			}

			auto  camera            = scene->getCamera();
			float viewManipulateTop = minBound.y;
			ImGui::SetCursorPos({sceneViewSize.x - 96, 32});
			auto entity = Application::getExecutePoint()->getEntityByName("MeshRoot");
			if (entity)
			{
				auto &transform = entity.getComponent<component::Transform>();
				auto  quat      = glm::quat(transform.getLocalOrientation());
				ImGui::gizmo3D("##gizmo2", quat, 96);
			}
		}
		ImGui::End();
	}

	auto PreviewWindow::resize(uint32_t width, uint32_t height) -> void
	{
		bool resized = false;
		if (this->width != width || this->height != height)
		{
			resized      = true;
			this->width  = width;
			this->height = height;
		}

		if (renderTexture == nullptr)
		{
			renderTexture = Texture2D::create();
			depthTexture  = TextureDepth::create(width, height);
		}
		if (resized)
		{
			auto camera = scene->getCamera();
			camera.first->setAspectRatio(width / (float) height);
			renderTexture->buildTexture(TextureFormat::RGBA8, width, height, false, false, false);
			depthTexture->resize(width, height);
			Application::getRenderGraph()->setPreview(renderTexture, depthTexture);
		}
	}

	auto PreviewWindow::handleInput(float dt) -> void
	{
		auto       group    = Application::getExecutePoint()->getRegistry().view<component::EditorCameraController>();
		const auto mousePos = Input::getInput()->getMousePosition();
		if (previousCurserPos == glm::vec2{0, 0})
		{
			previousCurserPos = mousePos;
		}
		for (auto &entity : group)
		{
			auto &controller = group.get<component::EditorCameraController>(entity);
			auto  camera     = scene->getCamera();
			controller.updateScroll(*camera.second, Input::getInput()->getScrollOffset(), dt);
			handleRotation(dt, mousePos);
		}
		constexpr float rotateDampeningFactor = 0.0000001f;
		previousCurserPos                     = mousePos;
		rotateVelocity                        = rotateVelocity * std::pow(rotateDampeningFactor, dt);
	}

	auto PreviewWindow::handleRotation(float dt, const glm::vec2 &pos) -> void
	{
		auto entity = Application::getExecutePoint()->getEntityByName("MeshRoot");
		if (Input::getInput()->isMouseHeld(KeyCode::MouseKey::ButtonLeft) && entity)
		{
			auto &transform        = entity.getComponent<component::Transform>();
			float mouseSensitivity = 0.02f;
			rotateVelocity         = rotateVelocity + (pos - previousCurserPos) * mouseSensitivity;
			glm::vec3 euler        = glm::degrees(transform.getLocalOrientation());
			float     pitch        = euler.x + rotateVelocity.y;
			float     yaw          = euler.y + rotateVelocity.x;
			transform.setLocalOrientation(glm::radians(glm::vec3{pitch, yaw, 0.0f}));
		}
	}

};        // namespace maple
