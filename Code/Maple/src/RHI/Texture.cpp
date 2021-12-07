//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "Texture.h"
#include "Others/Console.h"
#include "RHI/OpenGL/GLTexture.h"

namespace maple
{
	auto Texture::getStrideFromFormat(TextureFormat format) -> uint8_t
	{
		switch (format)
		{
			case TextureFormat::RGB:
				return 3;
			case TextureFormat::RGBA:
				return 4;
			case TextureFormat::R8:
				return 1;
			case TextureFormat::RG8:
				return 2;
			case TextureFormat::RGB8:
				return 3;
			case TextureFormat::RGBA8:
				return 4;
			default:
				return 0;
		}
	}

	auto Texture::bitsToTextureFormat(uint32_t bits) -> TextureFormat
	{
		switch (bits)
		{
			case 8:
				return TextureFormat::R8;
			case 16:
				return TextureFormat::RG8;
			case 24:
				return TextureFormat::RGB8;
			case 32:
				return TextureFormat::RGBA8;
			case 48:
				return TextureFormat::RGB16;
			case 64:
				return TextureFormat::RGBA16;
			default:
				MAPLE_ASSERT(false, "Unsupported image bit-depth!");
				return TextureFormat::RGB8;
		}
	}

	auto Texture::calculateMipMapCount(uint32_t width, uint32_t height) -> uint32_t
	{
		uint32_t levels = 1;
		while ((width | height) >> levels)
			levels++;
		return levels;
	}

	//###################################################

	auto Texture2D::create() -> std::shared_ptr<Texture2D>
	{
		return std::make_shared<GLTexture2D>();
	}

	auto Texture2D::create(uint32_t width, uint32_t height, void *data, TextureParameters parameters, TextureLoadOptions loadOptions) -> std::shared_ptr<Texture2D>
	{
		return std::make_shared<GLTexture2D>(width, height, data, parameters, loadOptions);
	}

	auto Texture2D::create(const std::string &name, const std::string &filePath, TextureParameters parameters, TextureLoadOptions loadOptions) -> std::shared_ptr<Texture2D>
	{
		return std::make_shared<GLTexture2D>(name, filePath, parameters, loadOptions);
	}

	auto Texture2D::getDefaultTexture() -> std::shared_ptr<Texture2D>
	{
		static std::shared_ptr<Texture2D> defaultTexture = create("default", "textures/default.png");
		return defaultTexture;
	}
	//###################################################

	auto TextureDepth::create(uint32_t width, uint32_t height, bool stencil) -> std::shared_ptr<TextureDepth>
	{
		return std::make_shared<GLTextureDepth>(width, height, stencil);
	}
	///#####################################################################################
	auto TextureCube::create(uint32_t size) -> std::shared_ptr<TextureCube>
	{
		return std::make_shared<GLTextureCube>(size);
	}

	auto TextureCube::create(uint32_t size, TextureFormat format, int32_t numMips) -> std::shared_ptr<TextureCube>
	{
		return std::make_shared<GLTextureCube>(size, format, numMips);
	}

	auto TextureCube::createFromFile(const std::string &filePath) -> std::shared_ptr<TextureCube>
	{
		return std::make_shared<GLTextureCube>(filePath);
	}

	auto TextureCube::createFromFiles(const std::array<std::string, 6> &files) -> std::shared_ptr<TextureCube>
	{
		return std::make_shared<GLTextureCube>(files);
	}

	auto TextureCube::createFromVCross(const std::vector<std::string> &files, uint32_t mips, TextureParameters params, TextureLoadOptions loadOptions, InputFormat format) -> std::shared_ptr<TextureCube>
	{
		return std::make_shared<GLTextureCube>(files, mips, params, loadOptions, format);
	}

	///#####################################################################################
	auto TextureDepthArray::create(uint32_t width, uint32_t height, uint32_t count) -> std::shared_ptr<TextureDepthArray>
	{
		return std::make_shared<GLTextureDepthArray>(width, height, count);
	}

}        // namespace maple
