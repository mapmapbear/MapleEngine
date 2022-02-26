//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <imgui.h>
#include <memory>
#include <string>
#include <IconsMaterialDesignIcons.h>
#include <imgui_node_editor.h>
#include <vector>
#include <unordered_map>
#include "EditorWindow.h"
namespace maple
{
	namespace ed = ax::NodeEditor;

	namespace capture_graph 
	{
		struct GraphNode;
	}

	class RenderGraphWindow : public EditorWindow
	{
	public:
		static constexpr char* STATIC_NAME = ICON_MDI_GRAPHQL "Render Graph";

		RenderGraphWindow();
		~RenderGraphWindow();
		virtual auto onImGui() -> void;

		struct NodeInfo
		{
			ed::NodeId nodeId;
			std::unordered_map<std::string, ed::PinId> inputs;
			std::unordered_map<std::string, ed::PinId> outputs;
			std::shared_ptr<capture_graph::GraphNode> graphNode;
		};

		struct Link
		{
			ed::LinkId ID;

			ed::PinId StartPinID;
			ed::PinId EndPinID;

			Link(ed::LinkId id, ed::PinId startPinId, ed::PinId endPinId) :
				ID(id), StartPinID(startPinId), EndPinID(endPinId)
			{
			}
		};

	private:
		auto gatherData() -> void;

		auto leftPanel(float w) -> void;
		auto rightPanel() -> void;
		auto splitter(bool splitVertically, float thickness, float* size1, float* size2, float minSize1, float minSize2, float splitterLongAxisSize = -1.0f)->bool;

		std::unordered_map<std::string, NodeInfo> idMap;
		std::vector< Link > links;
		ed::EditorContext* context = nullptr;

		NodeInfo* current = nullptr;
	};
};        // namespace maple
