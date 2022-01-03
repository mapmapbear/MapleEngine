//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "RHI/Texture.h"
#include <array>
#include <glm/glm.hpp>
#include <memory>

namespace maple
{
	enum class TextureFormat;
	enum GBufferTextures
	{
		COLOR       = 0,        //Main Render
		POSITION    = 1,        //Deferred Render - World Space Positions
		NORMALS     = 2,        //Deferred Render - World Space Normals
		PBR         = 3,
		SSAO_SCREEN = 4,
		SSAO_BLUR   = 5,
		LENGTH
	};

	class GBuffer
	{
	  public:
		GBuffer(uint32_t width, uint32_t height);

		inline auto getWidth() const
		{
			return width;
		}
		inline auto getHeight() const
		{
			return height;
		}
		auto resize(uint32_t width, uint32_t height, CommandBuffer *commandBuffer = nullptr) -> void;
		auto buildTexture(CommandBuffer *commandBuffer = nullptr) -> void;

		inline auto getDepthBuffer()
		{
			return depthBuffer;
		}
		inline auto getBuffer(uint32_t index)
		{
			return screenTextures[index];
		}
		inline auto getFormat(uint32_t index)
		{
			return formats[index];
		}

		inline auto getSSAONoise() const
		{
			return ssaoNoiseMap;
		}
		static auto getGBufferTextureName(GBufferTextures index) -> const char *;

	  private:
		std::array<std::shared_ptr<Texture2D>, GBufferTextures::LENGTH> screenTextures;
		std::array<TextureFormat, GBufferTextures::LENGTH>              formats;
		std::shared_ptr<TextureDepth>                                   depthBuffer;
		std::shared_ptr<Texture2D>                                      ssaoNoiseMap;

	  private:
		uint32_t width;
		uint32_t height;
	};
}        // namespace maple
