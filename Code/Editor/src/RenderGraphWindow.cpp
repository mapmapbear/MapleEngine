//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "RenderGraphWindow.h"
#include "Engine/Camera.h"
#include "ImGui/ImGuiHelpers.h"
#include "RHI/FrameBuffer.h"
#include "RHI/GraphicsContext.h"
#include "RHI/Texture.h"

#include "Editor.h"


namespace maple
{
	RenderGraphWindow::RenderGraphWindow()
	{
	}
	auto RenderGraphWindow::onImGui() -> void
	{
		if (ImGui::Begin(STATIC_NAME, &active))
		{

		}
		ImGui::End();
	}
}        // namespace maple
