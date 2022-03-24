//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <imgui.h>
#include <memory>
#include <string>

#include "EditorWindow.h"

namespace maple
{
	class VisualizeCacheWindow : public EditorWindow
	{
	  public:
		static constexpr char *STATIC_NAME = ICON_MDI_MESSAGE_SETTINGS "Visualize Cache";
		VisualizeCacheWindow();
		virtual auto onImGui() -> void;
	  private:
	};
};        // namespace maple