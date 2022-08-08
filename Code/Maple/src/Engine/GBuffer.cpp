//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "GBuffer.h"
#include "Others/Randomizer.h"

namespace maple
{
	GBuffer::GBuffer(uint32_t width, uint32_t height, const CommandBuffer *commandBuffer) :
	    width(width),
	    height(height)
	{
		formats[PREV_DISPLAY]      = TextureFormat::RGBA8;
		formats[SCREEN]            = TextureFormat::RGBA32;
		formats[BLOOM_SCREEN]      = TextureFormat::RGBA32;
		formats[BLOOM_BLUR]        = TextureFormat::RGBA32;
		formats[SSAO_SCREEN]       = TextureFormat::RGB8;
		formats[SSAO_BLUR]         = TextureFormat::RGB8;
		formats[SSR_SCREEN]        = TextureFormat::RGBA32;
		formats[COLOR]             = TextureFormat::RGBA32;
		formats[POSITION]          = TextureFormat::RGBA32;
		formats[NORMALS]           = TextureFormat::RGBA32;
		formats[PBR]               = TextureFormat::RGBA32;
		formats[VELOCITY]          = TextureFormat::RG8;
		formats[COLOR_PONG]        = TextureFormat::RGBA32;
		formats[POSITION_PONG]     = TextureFormat::RGBA32;
		formats[NORMALS_PONG]      = TextureFormat::RGBA32;
		formats[PBR_PONG]          = TextureFormat::RGBA32;
		formats[VELOCITY]          = TextureFormat::RG8;
		formats[VELOCITY_PONG]     = TextureFormat::RG8;

		formats[VIEW_POSITION]     = TextureFormat::RGBA32;
		formats[VIEW_NORMALS]      = TextureFormat::RGBA32;
		formats[PSEUDO_SKY]        = TextureFormat::RGBA8;
		formats[INDIRECT_LIGHTING] = TextureFormat::RGBA32;
		//buildTexture(commandBuffer);
	}

	auto GBuffer::resize(uint32_t width, uint32_t height, const CommandBuffer *commandBuffer) -> void
	{
		this->width  = width;
		this->height = height;
		buildTexture(commandBuffer);
	}

	auto GBuffer::buildTexture(const CommandBuffer *commandBuffer) -> void
	{
		if (depthBuffer[0] == nullptr)
		{
			for (auto i = 0; i < GBufferTextures::LENGTH; i++)
			{
				screenTextures[i] = Texture2D::create();
				screenTextures[i]->setName(GBufferNames[i]);
			}

			depthBuffer[0] = TextureDepth::create(width, height, true, commandBuffer);
			depthBuffer[0]->setName("GBuffer-Depth-Ping");

			depthBuffer[1] = TextureDepth::create(width, height, true, commandBuffer);
			depthBuffer[1]->setName("GBuffer-Depth-Pong");

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
			screenTextures[i]->buildTexture(formats[i], width, height, false, false, false);
		}
		depthBuffer[0]->resize(width, height, commandBuffer);
		depthBuffer[1]->resize(width, height, commandBuffer);
	}

	auto GBuffer::getBuffer(uint32_t i) -> std::shared_ptr<Texture2D>
	{
		if (i <= VELOCITY)
		{
			std::shared_ptr<Texture2D> textures[2] = 
			{
			        screenTextures[i],
			        screenTextures[i + 5]
			};
			return textures[index];
		}

		return screenTextures[i];
	}

	auto GBuffer::getBufferWithoutPingPong(uint32_t index) -> std::shared_ptr<Texture2D>
	{
		return screenTextures[index];
	}

	auto GBuffer::pingPong() -> void
	{
		index = 1 - index;
	}
}        // namespace maple
