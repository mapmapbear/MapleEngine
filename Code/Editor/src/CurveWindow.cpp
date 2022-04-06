//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "CurveWindow.h"
#include "ImGui/ImGuiHelpers.h"

#include "RHI/FrameBuffer.h"
#include "RHI/GraphicsContext.h"
#include "RHI/Texture.h"
#include "RHI/Shader.h"

#include "Engine/Camera.h"
#include "Loaders/Loader.h"
#include "Editor.h"

#include "Animation/Animation.h"
#include "Animation/AnimationSystem.h"
#include "Animation/Animator.h"

#include <implot.h>

namespace maple
{

	CurveWindow::CurveWindow()
	{
		active = true;
	}
	auto CurveWindow::onImGui() -> void
	{
		if (active) 
		{
			if (ImGui::Begin(STATIC_NAME, &active))
			{
				if (ImPlot::BeginPlot("##Plot", 0, 0, ImVec2(-1, -1), 0, ImPlotAxisFlags_NoInitialFit, ImPlotAxisFlags_NoInitialFit)) 
				{
				
					ImPlot::EndPlot();
				}
			}
			ImGui::End();
		}
	}
}        // namespace maple
