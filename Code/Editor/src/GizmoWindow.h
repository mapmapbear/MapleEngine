//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma  once
#include <string>
#include <imgui.h>
#include <memory>

#include "EditorWindow.h"

namespace maple 
{
	class Texture2D;
	class GizmoWindow : public EditorWindow
	{
	public:
		static constexpr char *STATIC_NAME = ICON_MDI_PINE_TREE "Gizmo";

		GizmoWindow();
		virtual auto onImGui() -> void ;
	private:
	};
};