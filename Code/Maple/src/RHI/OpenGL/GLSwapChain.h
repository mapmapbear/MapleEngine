#pragma once
#include "RHI/SwapChain.h"
#include <vector>

namespace maple
{
	class GLTexture2D;

	class GLSwapChain : public SwapChain
	{
	  public:
		GLSwapChain(uint32_t width, uint32_t height);
		~GLSwapChain();

		auto init(bool vsync) -> bool override;
		auto getCurrentImage() -> std::shared_ptr<Texture> override;
		auto getCurrentBufferIndex() const -> uint32_t override;
		auto getSwapChainBufferCount() const -> size_t override;

		inline auto init(bool vsync, NativeWindow *window) -> bool override
		{
			return init(vsync);
		}

		inline auto getCurrentImageIndex() const -> uint32_t override
		{
			return 0;
		};
		inline auto getImage(uint32_t index) -> std::shared_ptr<Texture> override
		{
			return nullptr;
		};

		inline auto onResize(uint32_t width, uint32_t height) -> void
		{
			this->width  = width;
			this->height = height;
		}

		inline auto getWidth() const -> uint32_t
		{
			return width;
		}
		inline auto getHeight() const -> uint32_t
		{
			return height;
		}

	  private:
		std::vector<std::shared_ptr<GLTexture2D>> swapChainBuffers;

		uint32_t currentBuffer = 0;
		uint32_t width         = 0;
		uint32_t height        = 0;
	};
}        // namespace maple
