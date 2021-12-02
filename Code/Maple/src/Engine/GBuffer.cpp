//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "GBuffer.h"

namespace maple
{
	GBuffer::GBuffer(uint32_t width, uint32_t height) :
	    width(width), height(height)
	{
		buildTexture();
	}

	auto GBuffer::resize(uint32_t width, uint32_t height) -> void
	{
		this->width  = width;
		this->height = height;
		buildTexture();
	}

	auto GBuffer::buildTexture() -> void
	{
		if (depthBuffer == nullptr)
		{
			for (auto i = 0; i < GBufferTextures::LENGTH; i++)
			{
				screenTextures[i] = Texture2D::create();
			}
			depthBuffer = TextureDepth::create(width, height);
		}

		formats[COLOR]    = TextureFormat::RGBA8;
		formats[POSITION] = TextureFormat::RGBA16;
		formats[NORMALS]  = TextureFormat::RGBA16;
		formats[PBR]      = TextureFormat::RGBA16;

		screenTextures[COLOR]->buildTexture(formats[COLOR], width, height, false, false, false);
		screenTextures[POSITION]->buildTexture(formats[POSITION], width, height, false, true, false);
		screenTextures[NORMALS]->buildTexture(formats[NORMALS], width, height, false, true, false);
		screenTextures[PBR]->buildTexture(formats[PBR], width, height, false, true, false);
		depthBuffer->resize(width, height);
	}

	auto GBuffer::getGBufferTextureName(GBufferTextures index) -> const char *
	{
		switch (index)
		{
#define STR(r) \
	case r:    \
		return #r
			STR(DEPTH);
			STR(COLOR);
			STR(POSITION);
			STR(NORMALS);
			STR(PBR);
#undef STR
			default:
				return "UNKNOWN_ERROR";
		}
	}

}        // namespace maple
