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
		COLOR,           //Main Render
		POSITION,        //Deferred Render - World Space Positions
		NORMALS,         //Deferred Render - World Space Normals
		PBR,
		SSAO_SCREEN,
		SSAO_BLUR,
		SSR_SCREEN,
		BLOOM_SCREEN,
		BLOOM_BLUR,

		SCREEN,
		INDIRECT_LIGHTING,
		PREV_DISPLAY,
		VIEW_POSITION,        //Deferred Render - View Space Positions  need to be optimized. they can be performed very well in post processing
		VIEW_NORMALS,
		VELOCITY,
		VOLUMETRIC_LIGHT,
		PSEUDO_SKY,
		LENGTH
	};

	static constexpr char *GBufferNames[] =
	    {
	        "Color",
	        "Position",
	        "Normals",
	        "PBR",
	        "SSAOScreen",
	        "SSAOBlur",
	        "SSRScreen",
	        "BloomScreen",
	        "BloomBlur",
	        "Screen",
	        "IndirectLighting",
	        "PreviewDisplay",
	        "ViewPosition",
	        "ViewNormal",
	        "Velocity",
	        "VolumetricLight",
	        "PseudoSky",
	        nullptr};

	class GBuffer
	{
	  public:
		GBuffer(uint32_t width, uint32_t height, const CommandBuffer *commandBuffer = nullptr);

		inline auto getWidth() const
		{
			return width;
		}
		inline auto getHeight() const
		{
			return height;
		}
		auto resize(uint32_t width, uint32_t height, const CommandBuffer *commandBuffer = nullptr) -> void;
		auto buildTexture(const CommandBuffer *commandBuffer = nullptr) -> void;

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
