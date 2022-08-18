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
		LINEARZ,
		VELOCITY,

		COLOR_PONG,   
		POSITION_PONG,        //Deferred Render - World Space Positions
		NORMALS_PONG,         //Deferred Render - World Space Normals
		PBR_PONG,
		LINEARZ_PONG,
		VELOCITY_PONG,

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
		PSEUDO_SKY,
		LENGTH
	};

	static constexpr char *GBufferNames[] =
	    {
	        "Color",
	        "Position",
	        "Normals",
	        "PBR",
	        "LinearZ",
	        "Velocity",

	        "Color-Pong",
	        "Position-Pong",
	        "Normals-Pong",
	        "PBR-Pong",
	        "LinearZ-Pong",
	        "Velocity-Pong",

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
	        "PseudoSky",
	        nullptr};

	enum PingPong
	{
		Ping,
		Pong
	};

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
			return depthBuffer[index];
		}

		inline auto getDepthBufferPong()
		{
			return depthBuffer[1 - index];
		}

		auto getBuffer(uint32_t index,bool pong = false) -> std::shared_ptr<Texture2D>;

		auto getBufferWithoutPingPong(uint32_t index) -> std::shared_ptr<Texture2D>;

		inline auto getFormat(uint32_t index)
		{
			return formats[index];
		}

		inline auto getSSAONoise() const
		{
			return ssaoNoiseMap;
		}

		auto pingPong() -> void;

		inline auto getIndex() const
		{
			return index;
		}

	  private:
		std::array<std::shared_ptr<Texture2D>, GBufferTextures::LENGTH> screenTextures;
		std::array<TextureFormat, GBufferTextures::LENGTH>              formats;
		std::shared_ptr<TextureDepth>                                   depthBuffer[2];
		std::shared_ptr<Texture2D>                                      ssaoNoiseMap;

	  private:
		uint32_t width;
		uint32_t height;
		int32_t  index = PingPong::Ping;
	};
}        // namespace maple
