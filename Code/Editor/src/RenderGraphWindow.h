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

	class RenderGraphWindow : public EditorWindow
	{
	  public:
		static constexpr char *STATIC_NAME = ICON_MDI_GRAPHQL "Render Graph";

		RenderGraphWindow();
		~RenderGraphWindow();
		virtual auto onImGui() -> void;
	  private:
		  std::unordered_map<std::string, ed::NodeId> idMap;
		ed::EditorContext* context = nullptr;
		int32_t idStart = 0;
	};
};        // namespace maple
