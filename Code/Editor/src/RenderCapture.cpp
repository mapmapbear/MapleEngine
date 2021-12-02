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
			for (auto &frame : Application::getGraphicsContext()->getFrameBufferCache())
			{
				auto attach = frame.second->getColorAttachment();
				if (attach)
				{
					ImGui::Image(attach->getHandle(), ImVec2(64, 64), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f), {1, 1, 1, 1}, {1,1,1,1});
				}

				//	ImGuiHelper::image(attach.get(), {64, 64});
			}
		}
		ImGui::End();
	}
}        // namespace maple
