#pragma once

#include "RHI/ImGuiRenderer.h"

namespace Maple
{
	class GLImGuiRenderer : public ImGuiRenderer
	{
	  public:
		GLImGuiRenderer(uint32_t width, uint32_t height, bool clearScreen);
		~GLImGuiRenderer();

		auto init() -> void override;
		auto newFrame(const Timestep &dt) -> void override;
		auto render(CommandBuffer *commandBuffer) -> void override;
		auto onResize(uint32_t width, uint32_t height) -> void override;
		auto rebuildFontTexture() -> void override;

	  private:
		void *windowHandle = nullptr;
		bool  clearScreen  = false;
	};
}        // namespace Maple
