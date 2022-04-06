//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <imgui.h>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "EditorWindow.h"

namespace maple
{
	class CurveWindow : public EditorWindow
	{
	  public:
		static constexpr char *STATIC_NAME = ICON_MDI_VECTOR_CURVE "Curve Editor";
		CurveWindow();
		virtual auto onImGui() -> void;
	};
};        // namespace maple