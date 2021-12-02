//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <memory>

namespace maple
{
	class Window;
	class Texture;
	class FrameBuffer;
	class RenderPass;
	class CommandBuffer;

	class SwapChain
	{
	  public:
		virtual ~SwapChain() = default;
		static auto create(uint32_t width, uint32_t height) -> std::shared_ptr<SwapChain>;

		virtual auto init(bool vsync, Window *window) -> bool             = 0;
		virtual auto init(bool vsync) -> bool                             = 0;
		virtual auto getCurrentImage() -> std::shared_ptr<Texture>        = 0;
		virtual auto getImage(uint32_t index) -> std::shared_ptr<Texture> = 0;
		virtual auto getCurrentBufferIndex() const -> uint32_t            = 0;
		virtual auto getCurrentImageIndex() const -> uint32_t             = 0;
		virtual auto getSwapChainBufferCount() const -> size_t            = 0;
		virtual auto onResize(uint32_t w, uint32_t h) -> void             = 0;
		virtual auto getCurrentCommandBuffer() -> CommandBuffer *
		{
			return nullptr;
		}
	};
}        // namespace maple
