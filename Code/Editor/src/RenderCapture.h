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
	class RenderCapture : public EditorWindow
	{
	  public:
		RenderCapture();
		virtual auto onImGui() -> void;
	  private:
	};
};        // namespace maple