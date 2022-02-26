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

#include <imgui.h>
#include <IconsMaterialDesignIcons.h>

namespace maple
{

	namespace
	{
		static int32_t NextId = 1;

		inline auto getNextId()
		{
			return NextId++;
		}

		inline auto drawNode(RenderGraphWindow::NodeInfo * node)
		{
			ed::BeginNode(node->nodeId);
			auto size = node->inputs.size();
			uint32_t i = 0;
			for (auto& pin : node->inputs)
			{
				ed::BeginPin(pin.second, ed::PinKind::Input);
				ImGui::TextUnformatted(ICON_MDI_ARROW_DOWN);
				ed::EndPin();
				i++;
				if (i < size)
				{
					ImGui::SameLine();
				}
			}

			ImGui::TextUnformatted(node->graphNode->name.c_str());
			if (node->graphNode->texture) 
			{
				ImGuiHelper::image(node->graphNode->texture.get(), { 128,128 });
			}

			size = node->graphNode->outputs.size();
			i = 0;
			for (auto& pin : node->outputs)
			{
				ed::BeginPin(pin.second, ed::PinKind::Output);
				ImGui::TextUnformatted(ICON_MDI_ARROW_DOWN);
				ed::EndPin();
				i++;
				if (i < size)
				{
					ImGui::SameLine();
				}
			}
			ed::EndNode();
		}

		inline auto getPinID(const std::string& name, std::unordered_map<std::string, ed::PinId> & map)
		{
			auto id = map.find(name);

			if (id == map.end())
			{
				id = map.emplace(name, getNextId()).first;
			}

			return id->second;
		}

		inline auto getNodeID(const std::string& name, std::unordered_map<std::string, RenderGraphWindow::NodeInfo>& map)
		{
			auto id = map.find(name);

			if (id == map.end())
			{
				id = map.emplace(name, RenderGraphWindow::NodeInfo{ ed::NodeId(getNextId()) }).first;
			}
			return id;
		}
	}

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

	struct Link
	{
		ed::LinkId ID;

		ed::PinId StartPinID;
		ed::PinId EndPinID;

		ImColor Color;

		Link(ed::LinkId id, ed::PinId startPinId, ed::PinId endPinId) :
			ID(id), StartPinID(startPinId), EndPinID(endPinId), Color(255, 255, 255)
		{

		}
	};

	auto RenderGraphWindow::onImGui() -> void
	{
		gatherData();
		ed::SetCurrentEditor(context);
		int32_t linkId = 0;

		if (ImGui::Begin(STATIC_NAME, &active))
		{
			static float leftPaneWidth = 400.0f;
			static float rightPaneWidth = 800.0f;
			splitter(true, 4.0f, &leftPaneWidth, &rightPaneWidth, 30.0f, 70.0f);

			leftPanel(leftPaneWidth - 4.f);

			ImGui::SameLine(0.0f, 12.0f);

			rightPanel();
		}
		ImGui::End();
	}

	auto RenderGraphWindow::gatherData() -> void
	{
		auto& editor = *static_cast<Editor*>(Application::get());
		auto& graph = editor.getCurrentScene()->getGlobalComponent<capture_graph::component::RenderGraph>();


		if (graph.nodes.size() != idMap.size()) 
		{
			links.clear();

			for (auto& node : graph.nodes)
			{
				auto iter = getNodeID(node.first, idMap);

				iter->second.graphNode = node.second;

				for (auto input : node.second->inputs)
				{
					auto toId = getPinID(input->name, iter->second.inputs);
					//create outputId in targert;
					auto fromNode = getNodeID(input->name, idMap);
					auto fromId = getPinID(node.first, fromNode->second.outputs);

					links.emplace_back(getNextId(), fromId, toId);
				}

				for (auto output : node.second->outputs)
				{
					auto fromId = getPinID(output->name, iter->second.outputs);

					auto toNode = getNodeID(output->name, idMap);
					auto toId = getPinID(node.first, toNode->second.inputs);

					links.emplace_back(getNextId(), fromId, toId);
				}
			}
		}
	}

	auto RenderGraphWindow::leftPanel(float paneWidth) -> void
	{
		auto& io = ImGui::GetIO();

		ImGui::BeginChild("Selection", ImVec2(paneWidth, 0));

		paneWidth = ImGui::GetContentRegionAvailWidth();

		for (auto & node : idMap)
		{
			if (node.second.graphNode->nodeType == capture_graph::NodeType::RenderPass) 
			{
				if (ImGui::Selectable(node.first.c_str())) 
				{
					current = &node.second;
				}
			}
		}
		
		if (ImGui::TreeNode("Textures Nodes")) 
		{
			for (auto& node : idMap)
			{
				if (node.second.graphNode->nodeType == capture_graph::NodeType::Image)
				{
					ImGui::Selectable(node.first.c_str());

					ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
					if (ImGui::IsItemHovered())
					{
						auto ratio = node.second.graphNode->texture->getWidth() / (float)node.second.graphNode->texture->getHeight();

						ImGui::BeginTooltip();
						ImGuiHelper::image(node.second.graphNode->texture.get(), { 512,512 / ratio });
						ImGui::EndTooltip();
					}
					ImGui::PopStyleVar();

				}
			}
			ImGui::TreePop();
		}

		ImGui::EndChild();
	}

	auto RenderGraphWindow::rightPanel() -> void
	{
		ed::Begin("Render Graph Window");

		auto cursorTopLeft = ImGui::GetCursorScreenPos();
		
		if (current != nullptr) 
		{
			drawNode(current);
			for (auto & input : current->inputs)
			{
				auto iter = getNodeID(input.first, idMap);
				drawNode(&iter->second);
			}

			for (auto& input : current->outputs)
			{
				auto iter = getNodeID(input.first, idMap);
				drawNode(&iter->second);
			}
		}
		else 
		{
			for (auto& node : idMap)
			{
				ed::BeginNode(node.second.nodeId);

				auto size = node.second.inputs.size();
				uint32_t i = 0;
				for (auto& pin : node.second.inputs)
				{
					ed::BeginPin(pin.second, ed::PinKind::Input);
					ImGui::TextUnformatted(ICON_MDI_ARROW_DOWN);
					ed::EndPin();
					i++;
					if (i < size)
					{
						ImGui::SameLine();
					}
				}

				ImGui::TextUnformatted(node.first.c_str());
				if (node.second.graphNode->texture)
					ImGuiHelper::image(node.second.graphNode->texture.get(), { 128,128 });

				size = node.second.outputs.size();
				i = 0;
				for (auto& pin : node.second.outputs)
				{
					ed::BeginPin(pin.second, ed::PinKind::Output);
					ImGui::TextUnformatted(ICON_MDI_ARROW_DOWN);
					ed::EndPin();
					i++;
					if (i < size)
					{
						ImGui::SameLine();
					}
				}

				ed::EndNode();
			}
		}

		for (auto& link : links)
			ed::Link(link.ID, link.StartPinID, link.EndPinID);

		ed::End();
	}

	auto RenderGraphWindow::splitter(bool splitVertically, float thickness, float* size1, float* size2, float minSize1, float minSize2, float splitterLongAxisSize /*= -1.0f*/) ->bool
	{
		using namespace ImGui;
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		ImGuiID id = window->GetID("##Splitter");
		ImRect bb;
		bb.Min = window->DC.CursorPos + (splitVertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
		bb.Max = bb.Min + CalcItemSize(splitVertically ? ImVec2(thickness, splitterLongAxisSize) : ImVec2(splitterLongAxisSize, thickness), 0.0f, 0.0f);
		return SplitterBehavior(bb, id, splitVertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, minSize1, minSize2, 0.0f);
	}

}        // namespace maple
