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

		formats[DISPLYA_0]    = TextureFormat::RGBA8;
		formats[PREV_DISPLAY] = TextureFormat::RGBA8;
		formats[SSAO_SCREEN]  = TextureFormat::RGB8;
		formats[SSAO_BLUR]    = TextureFormat::RGB8;
		formats[COLOR]        = TextureFormat::RGBA8;
		formats[POSITION]     = TextureFormat::RGBA16;
		formats[NORMALS]      = TextureFormat::RGBA16;

		formats[VIEW_POSITION] = TextureFormat::RGBA16;
		formats[VIEW_NORMALS]  = TextureFormat::RGBA16;
		formats[VELOCITY]      = TextureFormat::RGBA8;

		formats[PBR] = TextureFormat::RGBA16;

		screenTextures[VELOCITY]->buildTexture(formats[VELOCITY], width, height, false, false, false);
		screenTextures[DISPLYA_0]->buildTexture(formats[DISPLYA_0], width, height, false, false, false);
		screenTextures[PREV_DISPLAY]->buildTexture(formats[PREV_DISPLAY], width, height, false, false, false);
		screenTextures[SSAO_BLUR]->buildTexture(formats[SSAO_BLUR], width, height, false, false, false);
		screenTextures[SSAO_SCREEN]->buildTexture(formats[SSAO_SCREEN], width, height, false, false, false);
		screenTextures[COLOR]->buildTexture(formats[COLOR], width, height, false, false, false);
		screenTextures[POSITION]->buildTexture(formats[POSITION], width, height, false, false, false);
		screenTextures[NORMALS]->buildTexture(formats[NORMALS], width, height, false, false, false);

		screenTextures[VIEW_POSITION]->buildTexture(formats[VIEW_POSITION], width, height, false, false, false);
		screenTextures[VIEW_NORMALS]->buildTexture(formats[VIEW_NORMALS], width, height, false, false, false);

		screenTextures[PBR]->buildTexture(formats[PBR], width, height, false, false, false);
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
#undef STR
			default:
				return "UNKNOWN_ERROR";
		}
	}

	auto GBuffer::swapScreenBuffer() -> void
	{
		auto display                 = screenTextures[DISPLYA_0];
		screenTextures[DISPLYA_0]    = screenTextures[PREV_DISPLAY];
		screenTextures[PREV_DISPLAY] = display;
	}

}        // namespace maple
