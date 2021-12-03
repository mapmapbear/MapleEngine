//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "RenderCapture.h"
#include "ImGui/ImGuiHelpers.h"
#include "RHI/FrameBuffer.h"
#include "RHI/GraphicsContext.h"
#include "RHI/Texture.h"

#include "Application.h"

namespace maple
{
	RenderCapture::RenderCapture()
	{
		title = "RenderCapture";
	}
	auto RenderCapture::onImGui() -> void
	{
		ImGui::Begin(title.c_str(), &active);
		{
			Application::getRenderGraph()->onImGui();
		}
		ImGui::End();
	}
}        // namespace maple
