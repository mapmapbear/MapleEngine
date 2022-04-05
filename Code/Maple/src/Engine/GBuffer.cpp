//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "GBuffer.h"
#include "Others/Randomizer.h"

namespace maple
{
	GBuffer::GBuffer(uint32_t width, uint32_t height) :
	    width(width), height(height)
	{
		formats[PREV_DISPLAY]     = TextureFormat::RGBA8;
		formats[SCREEN]           = TextureFormat::RGBA32;
		formats[BLOOM_SCREEN]     = TextureFormat::RGBA32;
		formats[BLOOM_BLUR]		  = TextureFormat::RGBA32;
		formats[SSAO_SCREEN]      = TextureFormat::RGB8;
		formats[SSAO_BLUR]        = TextureFormat::RGB8;
		formats[SSR_SCREEN]       = TextureFormat::RGBA32;
		formats[COLOR]            = TextureFormat::RGBA32;
		formats[POSITION]         = TextureFormat::RGBA32;
		formats[NORMALS]          = TextureFormat::RGBA32;
		formats[VIEW_POSITION]    = TextureFormat::RGBA32;
		formats[VIEW_NORMALS]     = TextureFormat::RGBA32;
		formats[VELOCITY]         = TextureFormat::RGBA8;
		formats[PBR]              = TextureFormat::RGBA32;
		formats[VOLUMETRIC_LIGHT] = TextureFormat::RGB8;
		formats[PSEUDO_SKY]       = TextureFormat::RGBA8;
		formats[INDIRECT_LIGHTING] = TextureFormat::RGBA32;
		buildTexture();
	}

	auto GBuffer::resize(uint32_t width, uint32_t height, CommandBuffer *commandBuffer) -> void
	{
		this->width  = width;
		this->height = height;
		buildTexture(commandBuffer);
	}

	auto GBuffer::buildTexture(CommandBuffer *commandBuffer) -> void
	{
		if (depthBuffer == nullptr)
		{
			for (auto i = 0; i < GBufferTextures::LENGTH; i++)
			{
				screenTextures[i] = Texture2D::create();
				screenTextures[i]->setName(getGBufferTextureName((GBufferTextures) i));
			}
			depthBuffer = TextureDepth::create(width, height, true);
			depthBuffer->setName("GBuffer-Depth");
#if defined(__ANDROID__)
			constexpr int32_t SSAO_NOISE_DIM = 8;
#else
			constexpr int32_t SSAO_NOISE_DIM = 4;
#endif
			std::vector<glm::vec4> ssaoNoise(SSAO_NOISE_DIM * SSAO_NOISE_DIM);
			for (uint32_t i = 0; i < static_cast<uint32_t>(ssaoNoise.size()); i++)
			{
				ssaoNoise[i] = glm::vec4(Randomizer::random() * 2.0f - 1.0f, Randomizer::random() * 2.0f - 1.0f, 0.0f, 0.0f);
			}
			ssaoNoiseMap = Texture2D::create(SSAO_NOISE_DIM, SSAO_NOISE_DIM, ssaoNoise.data(), {TextureFormat::RGBA32, TextureFilter::Nearest, TextureWrap::ClampToEdge});
			ssaoNoiseMap->setName("SSAO-NoiseMap");
		}

		for (int32_t i = COLOR; i < LENGTH; i++)
		{
			if (i == VOLUMETRIC_LIGHT)
			{
				screenTextures[i]->buildTexture(formats[i], width / 2.f, height / 2.f, false, false, false);
			}
			else
			{
				screenTextures[i]->buildTexture(formats[i], width, height, false, false, false);
			}
		}
		depthBuffer->resize(width, height, commandBuffer);
	}

	auto GBuffer::getGBufferTextureName(GBufferTextures index) -> const char *
	{
		switch (index)
		{
#define STR(r) \
	case r:    \
		return #r
			STR(COLOR);
			STR(POSITION);
			STR(NORMALS);
			STR(PBR);
			STR(BLOOM_SCREEN);
			STR(SSAO_SCREEN);
			STR(SSAO_BLUR);
			STR(SSR_SCREEN);
			STR(SCREEN);
			STR(VIEW_POSITION);
			STR(VIEW_NORMALS);
			STR(VELOCITY);
			STR(INDIRECT_LIGHTING);
			STR(PSEUDO_SKY);
			STR(BLOOM_BLUR);
#undef STR
			default:
				return "UNKNOWN_ERROR";
		}
	}
}        // namespace maple
