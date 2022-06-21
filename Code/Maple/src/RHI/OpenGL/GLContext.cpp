//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "GLContext.h"
#include "Application.h"
#include "GL.h"
#include "RHI/SwapChain.h"
#include <imgui/imgui.h>

namespace maple
{
	GLContext::GLContext()
	{
	}

	GLContext::~GLContext() = default;

	auto GLContext::present() -> void
	{
	}

	auto GLContext::onImGui() -> void
	{
		ImGui::TextUnformatted("%s", (const char *) (glGetString(GL_VERSION)));
		ImGui::TextUnformatted("%s", (const char *) (glGetString(GL_VENDOR)));
		ImGui::TextUnformatted("%s", (const char *) (glGetString(GL_RENDERER)));
	}

	auto GLContext::init() -> void
	{
		auto &window = Application::getWindow();
		swapChain    = SwapChain::create(window->getWidth(), window->getHeight());
		swapChain->init(false, window.get());
	}
}        // namespace maple
