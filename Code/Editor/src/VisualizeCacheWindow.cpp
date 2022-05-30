//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "VisualizeCacheWindow.h"
#include "ImGui/ImGuiHelpers.h"

#include "RHI/FrameBuffer.h"
#include "RHI/GraphicsContext.h"
#include "RHI/Texture.h"
#include "RHI/Shader.h"

#include "Engine/Camera.h"
#include "Loaders/Loader.h"

#include "Editor.h"

namespace maple
{
	namespace 
	{
		inline auto showEditorCamera() -> void
		{
			ImGui::TextUnformatted("Editor Transform");

			auto editor = static_cast<Editor*>(Editor::get());

			auto& transform = editor->getEditorCameraTransform();

			auto rotation = glm::degrees(transform.getLocalOrientation());
			auto position = transform.getLocalPosition();
			auto scale = transform.getLocalScale();

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
			ImGui::Separator();

		}

		inline auto drawCache() 
		{
			auto& cache = Application::getAssetsLoaderFactory()->getCache();
			ImGui::Columns(2);
			for (auto & res : cache)
			{
				ImGui::TextUnformatted(res.first.c_str());
				ImGui::NextColumn();
				ImGui::PushItemWidth(-1);
				
				for (auto & r : res.second)
				{
					if (r->getResourceType() == maple::FileType::Shader) 
					{
						ImGui::PushID(r.get());
						if (ImGui::Button("Reload")) 
						{
							auto shader = std::static_pointer_cast<Shader>(r);
							shader->reload();
						}
						ImGui::PopID();
					}
					else if (r->getResourceType() == maple::FileType::Texture) 
					{
						auto texture = std::static_pointer_cast<Texture>(r);
						ImGui::PushID(r.get());
					
						ImGui::Text(fileTypeToStr(r->getResourceType()));

						ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
						if (ImGui::IsItemHovered())
						{
							
							ImGui::BeginTooltip();
							ImGuiHelper::image(texture.get(), { 50,50 });
							ImGui::EndTooltip();
						}
						ImGui::PopStyleVar();

						ImGui::PopID();

						ImGui::SameLine();

						ImGui::Text(" TextureID : 0x%08hhx", texture->toIntID());
					}
					else if (r->getResourceType() == maple::FileType::Animation)
					{
						ImGui::PushID(r.get());

						if (ImGui::Button("Play"))
						{
							//auto animation = std::static_pointer_cast<Animation>(r);
						}

						ImGui::PopID();
					}
					else 
					{
						ImGui::TextUnformatted(fileTypeToStr(r->getResourceType()));
					}
				}

				ImGui::PopItemWidth();
				ImGui::Separator();
				ImGui::NextColumn();
			}
			ImGui::Columns(1);
			ImGui::Separator();
		}
	}


	VisualizeCacheWindow::VisualizeCacheWindow()
	{
	
	}
	auto VisualizeCacheWindow::onImGui() -> void
	{
		ImGui::Begin(STATIC_NAME, &active);
		{
			showEditorCamera();

			if (ImGui::TreeNode("Draw Cache"))
			{
				drawCache();
				ImGui::TreePop();
			}
		}
		ImGui::End();
	}
}        // namespace maple
