//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "RenderGraphWindow.h"
#include "Engine/Camera.h"
#include "ImGui/ImGuiHelpers.h"
#include "RHI/FrameBuffer.h"
#include "RHI/GraphicsContext.h"
#include "RHI/Texture.h"
#include "Scene/Scene.h"
#include "Engine/CaptureGraph.h"

#include "Editor.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>
#include <imgui.h>

namespace maple
{
	RenderGraphWindow::RenderGraphWindow()
	{
		ed::Config config;
		config.SettingsFile = "RenderGraph.json";
		context = ed::CreateEditor(&config);
		ed::SetCurrentEditor(context);
	}

	RenderGraphWindow::~RenderGraphWindow()
	{
		if (context != nullptr)
			ed::DestroyEditor(context);
	}

	auto RenderGraphWindow::onImGui() -> void
	{
		auto& editor = *static_cast<Editor*>(Application::get());

		auto& graph = editor.getCurrentScene()->getGlobalComponent<capture_graph::component::RenderGraph>();

		ed::SetCurrentEditor(context);
		if (ImGui::Begin(STATIC_NAME, &active))
		{
			ed::Begin("Render Graph Window");

            for (auto& node : graph.nodes)
            {
                if (node.second->nodeType != capture_graph::NodeType::RenderPass)
                    continue;

                const float rounding = 10.0f;
                const float padding = 12.0f;
                auto id = idMap.find(node.first);

                if (id == idMap.end()) 
                {
                  id = idMap.emplace(node.first, idStart++).first;
                }

                ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(229, 229, 229, 200));
                ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(125, 125, 125, 200));
                ed::PushStyleColor(ed::StyleColor_PinRect, ImColor(229, 229, 229, 60));
                ed::PushStyleColor(ed::StyleColor_PinRectBorder, ImColor(125, 125, 125, 60));

                const auto pinBackground = ed::GetStyle().Colors[ed::StyleColor_NodeBg];

                ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(0, 0, 0, 0));
                ed::PushStyleVar(ed::StyleVar_NodeRounding, rounding);
                ed::PushStyleVar(ed::StyleVar_SourceDirection, ImVec2(0.0f, 1.0f));
                ed::PushStyleVar(ed::StyleVar_TargetDirection, ImVec2(0.0f, -1.0f));
                ed::PushStyleVar(ed::StyleVar_LinkStrength, 0.0f);
                ed::PushStyleVar(ed::StyleVar_PinBorderWidth, 1.0f);
                ed::PushStyleVar(ed::StyleVar_PinRadius, 6.0f);
                ed::BeginNode(id->second);

                ImGui::Dummy(ImVec2(160, 0));
                ImGui::TextUnformatted(node.first.c_str());
               /* ImGui::BeginVertical(node.ID.AsPointer());
                if (!node.Inputs.empty())
                {
                    ImGui::BeginHorizontal("inputs");
                    ImGui::Spring(1, 0);

                    ImRect inputsRect;
                    int inputAlpha = 200;
                    for (auto& pin : node.Inputs)
                    {
                        ImGui::Dummy(ImVec2(padding, padding));
                        inputsRect = ImGui_GetItemRect();
                        ImGui::Spring(1, 0);
                        inputsRect.Min.y -= padding;
                        inputsRect.Max.y -= padding;

                        //ed::PushStyleVar(ed::StyleVar_PinArrowSize, 10.0f);
                        //ed::PushStyleVar(ed::StyleVar_PinArrowWidth, 10.0f);
                        ed::PushStyleVar(ed::StyleVar_PinCorners, 15);
                        ed::BeginPin(pin.ID, ed::PinKind::Input);
                        ed::PinPivotRect(inputsRect.GetCenter(), inputsRect.GetCenter());
                        ed::PinRect(inputsRect.GetTL(), inputsRect.GetBR());
                        ed::EndPin();
                        //ed::PopStyleVar(3);
                        ed::PopStyleVar(1);

                        auto drawList = ImGui::GetWindowDrawList();
                        drawList->AddRectFilled(inputsRect.GetTL(), inputsRect.GetBR(),
                            IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, 15);
                        drawList->AddRect(inputsRect.GetTL(), inputsRect.GetBR(),
                            IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, 15);

                        if (newLinkPin && !CanCreateLink(newLinkPin, &pin) && &pin != newLinkPin)
                            inputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
                    }

                    //ImGui::Spring(1, 0);
                    ImGui::EndHorizontal();
                }
                */

                /*ImGui::BeginHorizontal("content_frame");
                ImGui::Spring(1, padding);

                ImGui::BeginVertical("content", ImVec2(0.0f, 0.0f));
                ImGui::Dummy(ImVec2(160, 0));
                ImGui::Spring(1);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                ImGui::TextUnformatted(node.Name.c_str());
                ImGui::PopStyleColor();
                ImGui::Spring(1);
                ImGui::EndVertical();
                auto contentRect = ImGui_GetItemRect();

                ImGui::Spring(1, padding);
                ImGui::EndHorizontal();*/

                /*if (!node.Outputs.empty())
                {
                    ImGui::BeginHorizontal("outputs");
                    ImGui::Spring(1, 0);

                    ImRect outputsRect;
                    int outputAlpha = 200;
                    for (auto& pin : node.Outputs)
                    {
                        ImGui::Dummy(ImVec2(padding, padding));
                        outputsRect = ImGui_GetItemRect();
                        ImGui::Spring(1, 0);
                        outputsRect.Min.y += padding;
                        outputsRect.Max.y += padding;

                        ed::PushStyleVar(ed::StyleVar_PinCorners, 3);
                        ed::BeginPin(pin.ID, ed::PinKind::Output);
                        ed::PinPivotRect(outputsRect.GetCenter(), outputsRect.GetCenter());
                        ed::PinRect(outputsRect.GetTL(), outputsRect.GetBR());
                        ed::EndPin();
                        ed::PopStyleVar();

                        auto drawList = ImGui::GetWindowDrawList();
                        drawList->AddRectFilled(outputsRect.GetTL(), outputsRect.GetBR(),
                            IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, 15);
                        drawList->AddRect(outputsRect.GetTL(), outputsRect.GetBR(),
                            IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, 15);


                        if (newLinkPin && !CanCreateLink(newLinkPin, &pin) && &pin != newLinkPin)
                            outputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
                    }

                    ImGui::EndHorizontal();
                }*/

                //ImGui::EndVertical();

                ed::EndNode();
                ed::PopStyleVar(7);
                ed::PopStyleColor(4);
            }
			ed::End();
		}
		ImGui::End();
		ed::SetCurrentEditor(nullptr);
	}
}        // namespace maple
