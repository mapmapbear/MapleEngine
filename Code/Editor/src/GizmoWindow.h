//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma  once
#include <string>
#include <imgui.h>
#include <memory>

#include "EditorWindow.h"

namespace Maple 
{
	class Texture2D;
	class GizmoWindow : public EditorWindow
	{
	public:
		GizmoWindow();
		virtual auto onImGui() -> void ;
	private:
	};
};