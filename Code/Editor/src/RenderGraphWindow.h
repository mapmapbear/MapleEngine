//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <imgui.h>
#include <memory>
#include <string>

#include "EditorWindow.h"

#include <IconsMaterialDesignIcons.h>

namespace maple
{
	class RenderGraphWindow : public EditorWindow
	{
	  public:
		static constexpr char *STATIC_NAME = ICON_MDI_GRAPHQL "Render Graph";

		RenderGraphWindow();
		virtual auto onImGui() -> void;
	  private:
	};
};        // namespace maple