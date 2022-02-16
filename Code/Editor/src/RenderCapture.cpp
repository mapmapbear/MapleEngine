//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "RenderCapture.h"
#include "ImGui/ImGuiHelpers.h"
#include "RHI/FrameBuffer.h"
#include "RHI/GraphicsContext.h"
#include "RHI/Texture.h"
#include "Engine/Camera.h"

#include "Editor.h"

namespace maple
{
	RenderCapture::RenderCapture()
	{
	
	}
	auto RenderCapture::onImGui() -> void
	{
		ImGui::Begin(STATIC_NAME, &active);
		{
			Application::getRenderGraph()->onImGui();
			ImGui::Separator();
			ImGui::TextUnformatted("Editor Transform");

			auto editor = static_cast<Editor *>(Editor::get());

			auto &transform = editor->getEditorCameraTransform();

			auto rotation = glm::degrees(transform.getLocalOrientation());
			auto position = transform.getLocalPosition();
			auto scale    = transform.getLocalScale();

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
			ImGui::Columns(2);
			ImGui::Separator();

			ImGui::TextUnformatted("Position");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);

			if (ImGui::DragFloat3("##Position", &position.x, 0.05))
			{
				transform.setLocalPosition(position);
			}

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::TextUnformatted("Rotation");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);

			if (ImGui::DragFloat3("##Rotation", &rotation.x, 0.1))
			{
				transform.setLocalOrientation(glm::radians(rotation));
			}

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::TextUnformatted("Scale");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			if (ImGui::DragFloat3("##Scale", &scale.x, 0.05))
			{
				transform.setLocalScale(scale);
			}

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::Columns(2);
			ImGui::PopStyleVar();

			ImGui::Separator();

			ImGui::TextUnformatted("Rotation");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);

			if (ImGui::DragFloat3("##Rotation", &rotation.x, 0.1))
			{
				transform.setLocalOrientation(glm::radians(rotation));
			}

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::TextUnformatted("Far Plane");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);

			auto farPlane = editor->getCamera()->getFar();

			if (ImGui::DragFloat("##FarPlane", &farPlane, 0.05))
			{
				editor->getCamera()->setFar(farPlane);
			}
			ImGui::Columns(1);

		}
		ImGui::End();
	}
}        // namespace maple
