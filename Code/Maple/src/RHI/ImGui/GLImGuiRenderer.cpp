#include "GLImGuiRenderer.h"
#include "Application.h"
#include "RHI/OpenGL/GL.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace Maple
{
	GLImGuiRenderer::GLImGuiRenderer(uint32_t width, uint32_t height, bool clearScreen) :
	    clearScreen(clearScreen)
	{
	}

	GLImGuiRenderer::~GLImGuiRenderer()
	{
		ImGui_ImplOpenGL3_Shutdown();
	}

	auto GLImGuiRenderer::init() -> void
	{
		ImGui_ImplGlfw_InitForOpenGL((GLFWwindow*)Application::getWindow()->getNativeInterface(), true);
		ImGui_ImplOpenGL3_Init("#version 410");
	}

	auto GLImGuiRenderer::newFrame(const Timestep &dt) -> void
	{
		ImGuiIO &io  = ImGui::GetIO();
		io.DeltaTime = dt.getMilliseconds();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	auto GLImGuiRenderer::render(CommandBuffer *commandBuffer) -> void
	{
		ImGui::Render();
		if (clearScreen)
		{
			GLCall(glClear(GL_COLOR_BUFFER_BIT));
		}
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	auto GLImGuiRenderer::onResize(uint32_t width, uint32_t height) -> void
	{
	}

	auto GLImGuiRenderer::rebuildFontTexture() -> void
	{
		ImGui_ImplOpenGL3_DestroyFontsTexture();
		ImGui_ImplOpenGL3_CreateFontsTexture();
	}
}        // namespace Maple
