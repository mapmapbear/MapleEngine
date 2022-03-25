#include "GLTexture.h"
#include "GL.h"
#include "GLFrameBuffer.h"
#include "GLShader.h"

#include "Engine/Profiler.h"
#include "Others/Console.h"

#include "Loaders/ImageLoader.h"

namespace maple
{
	namespace
	{
		inline auto textureWrapToGL(const TextureWrap &wrap)
		{
			switch (wrap)
			{
#ifndef PLATFORM_MOBILE
				case TextureWrap::Clamp:
					return GL_CLAMP;
				case TextureWrap::ClampToBorder:
					return GL_CLAMP_TO_BORDER;
#endif
				case TextureWrap::ClampToEdge:
					return GL_CLAMP_TO_EDGE;
				case TextureWrap::Repeat:
					return GL_REPEAT;
				case TextureWrap::MirroredRepeat:
					return GL_MIRRORED_REPEAT;
				default:
					MAPLE_ASSERT(false, "[Texture] Unsupported TextureWrap");
					return 0;
			}
		}

		inline auto textureFormatToGL(const TextureFormat format, bool srgb)
		{
			switch (format)
			{
				case TextureFormat::RGBA:
					return GL_RGBA;
				case TextureFormat::RGB:
					return GL_RGB;
				case TextureFormat::R8:
					return GL_R8;
				case TextureFormat::RG8:
					return GL_RG8;
				case TextureFormat::RG16F:
					return GL_RG16F;
				case TextureFormat::RGB8:
					return srgb ? GL_SRGB8 : GL_RGB8;
				case TextureFormat::RGBA8:
					return srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
				case TextureFormat::RGB16:
					return GL_RGB16F;
				case TextureFormat::RGBA16:
					return GL_RGBA16F;
				case TextureFormat::RGB32:
					return GL_RGB32F;
				case TextureFormat::RGBA32:
					return GL_RGBA32F;
				case TextureFormat::DEPTH:
					return GL_DEPTH24_STENCIL8;
				case TextureFormat::R32I:
					return GL_R32I;
				case TextureFormat::R32UI:
					return GL_R32UI;
				default:
					MAPLE_ASSERT(false, "[Texture] Unsupported TextureFormat");
					return 0;
			}
		}



		inline auto textureFormatSize(const TextureFormat format)
		{
			switch (format)
			{
			case TextureFormat::RGBA8:
			case TextureFormat::RGBA:
				return sizeof(int32_t);
			case TextureFormat::RGB:
				return sizeof(int8_t) * 3;
			case TextureFormat::R8:
				return sizeof(int8_t);
			case TextureFormat::RG8:
				return sizeof(int16_t);
			case TextureFormat::RGB8:
				return sizeof(int8_t) * 3;
			case TextureFormat::RG16F:
				return sizeof(int16_t) * 2;
			case TextureFormat::RGB16:
				return sizeof(int16_t) * 3;
			case TextureFormat::RGBA16:
				return sizeof(int16_t) * 4;
			case TextureFormat::RGB32:
				return sizeof(int32_t) * 3;
			case TextureFormat::RGBA32:
				return sizeof(int32_t) * 4;
			case TextureFormat::R32I:
				return sizeof(int32_t);
			case TextureFormat::R32UI:
				return sizeof(uint32_t);
			default:
				MAPLE_ASSERT(false, "[Texture] Unsupported TextureFormat");
				return (uint64_t)0;
			}
		}


		inline auto textureDataType(const TextureFormat format)
		{
			switch (format) {
				case TextureFormat::R32I:
					return GL_INT;
				case TextureFormat::R32UI:
					return GL_UNSIGNED_INT;
				default:
					return GL_FLOAT;
			}
		}

		inline auto internalFormatToFormat(uint32_t format)
		{
			switch (format)
			{
				case GL_SRGB8:
					return GL_RGB;
				case GL_SRGB8_ALPHA8:
					return GL_RGBA;
				case GL_RGBA:
					return GL_RGBA;
				case GL_RGB:
					return GL_RGB;
				case GL_R8:
					return GL_RED;
				case GL_RG8:
					return GL_RG;
				case GL_RGB8:
					return GL_RGB;
				case GL_RGBA8:
					return GL_RGBA;
				case GL_RGB16:
					return GL_RGB;
				case GL_RG16F:
					return GL_RG;
				case GL_RGBA16:
					return GL_RGBA;
				case GL_RGBA16F:
					return GL_RGBA;
				case GL_RGB32F:
					return GL_RGB;
				case GL_RGBA32F:
					return GL_RGBA;
				case GL_SRGB:
					return GL_RGB;
				case GL_SRGB_ALPHA:
					return GL_RGBA;
				case GL_R32I:
				case GL_R32UI:
					return GL_RED_INTEGER;
				case GL_LUMINANCE:
					return GL_LUMINANCE;
				case GL_LUMINANCE_ALPHA:
					return GL_LUMINANCE_ALPHA;
				default:
					MAPLE_ASSERT(false, "[Texture] Unsupported Texture Format");
					return 0;
			}
		}
	};        // namespace

	GLTexture2D::GLTexture2D()
	{
		format = parameters.format;
		glGenTextures(1, &handle);
	}

	GLTexture2D::GLTexture2D(uint32_t width, uint32_t height, const void *data, TextureParameters parameters, TextureLoadOptions loadOptions) :
	    parameters(parameters), loadOptions(loadOptions), width(width), height(height)
	{
		format = parameters.format;
		load(data);
	}

	GLTexture2D::GLTexture2D(const std::string &initName, const std::string &fileName, const TextureParameters parameters, const TextureLoadOptions loadOptions) :
	    fileName(fileName), parameters(parameters), loadOptions(loadOptions)
	{
		name = initName;

		auto pixels = ImageLoader::loadAsset(fileName,loadOptions.generateMipMaps,loadOptions.flipY);

		format = pixels->getPixelFormat();
		width  = pixels->getWidth();
		height = pixels->getHeight();
		isHDR  = pixels->isHDR();

		this->parameters.format = format;
		load(pixels->getData());
	}

	GLTexture2D::~GLTexture2D()
	{
		PROFILE_FUNCTION();
		GLCall(glDeleteTextures(1, &handle));
	}

	auto GLTexture2D::load(const void *data) -> void
	{
		PROFILE_FUNCTION();
		const uint8_t *pixels = reinterpret_cast<const uint8_t *>(data);

		GLCall(glGenTextures(1, &handle));
		GLCall(glBindTexture(GL_TEXTURE_2D, handle));
		//need to be refactored
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, parameters.minFilter == TextureFilter::Linear ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, parameters.magFilter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, textureWrapToGL(parameters.wrap)));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, textureWrapToGL(parameters.wrapT)));

		uint32_t format = textureFormatToGL(parameters.format, parameters.srgb);

		auto floatData = parameters.format == TextureFormat::RGB16 || parameters.format == TextureFormat::RGB32 || parameters.format == TextureFormat::RGBA16 || parameters.format == TextureFormat::RGBA32;

		GLCall(glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, internalFormatToFormat(format), (isHDR || floatData) ? GL_FLOAT : GL_UNSIGNED_BYTE, data ? data : nullptr));
		GLCall(glGenerateMipmap(GL_TEXTURE_2D));
	}

	auto GLTexture2D::bind(uint32_t slot) const -> void
	{
		PROFILE_FUNCTION();
		GLCall(glActiveTexture(GL_TEXTURE0 + slot));
		GLCall(glBindTexture(GL_TEXTURE_2D, handle));
	}

	auto GLTexture2D::unbind(uint32_t slot) const -> void
	{
		PROFILE_FUNCTION();
		GLCall(glActiveTexture(GL_TEXTURE0 + slot));
		GLCall(glBindTexture(GL_TEXTURE_2D, 0));
	}

	auto GLTexture2D::buildTexture(TextureFormat internalformat, uint32_t w, uint32_t h, bool srgb, bool depth, bool samplerShadow, bool mipmap, bool image, uint32_t accessFlag) -> void
	{
		PROFILE_FUNCTION();
		format      = internalformat;
		width       = w;
		height      = h;
		if(name == "")
			name        = "Unknown-Attachment";
		storedImage = image;

		uint32_t glFormat  = textureFormatToGL(internalformat, srgb);
		uint32_t glFormat2 = internalFormatToFormat(glFormat);

		GLCall(glBindTexture(GL_TEXTURE_2D, handle));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

		if (samplerShadow)
		{
			GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL));
#ifndef PLATFORM_MOBILE
			GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE));
			GLCall(glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY));
#endif
		}

		GLCall(glTexImage2D(GL_TEXTURE_2D, 0, glFormat, width, height, 0, glFormat2, depth ? GL_UNSIGNED_BYTE : GL_FLOAT, nullptr));

		if (image)
		{
			bindImageTexture(0, accessFlag == 0 || accessFlag == 2, accessFlag == 1 || accessFlag == 2, 0, 0);        //should use mask
		}

		if (mipmap)
			GLCall(glGenerateMipmap(GL_TEXTURE_2D));
	}

	auto GLTexture2D::bindImageTexture(uint32_t unit, bool read /*= false*/, bool write /*= false*/, uint32_t level /*= 0*/, uint32_t layer /*= 0*/) -> void
	{
		PROFILE_FUNCTION();
		auto flag = read && write ? GL_READ_WRITE :
		            read          ? GL_READ_ONLY :
		            write         ? GL_WRITE_ONLY :
                                    GL_READ_WRITE;
		GLCall(glBindImageTexture(unit, handle, level, false, layer, flag, textureFormatToGL(format, false)));
	}

	auto GLTexture2D::update(int32_t x, int32_t y, int32_t w, int32_t h, const void *buffer) -> void
	{
		PROFILE_FUNCTION();
		GLCall(glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, internalFormatToFormat(textureFormatToGL(parameters.format, parameters.srgb)), isHDR ? GL_FLOAT : GL_UNSIGNED_BYTE, buffer));
	}

	auto GLTexture2D::setData(const void *pixels) -> void
	{
		PROFILE_FUNCTION();
		auto floatData = parameters.format == TextureFormat::RGB16 || parameters.format == TextureFormat::RGB32 || parameters.format == TextureFormat::RGBA16 || parameters.format == TextureFormat::RGBA32;

		GLCall(glBindTexture(GL_TEXTURE_2D, handle));
		GLCall(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, internalFormatToFormat(textureFormatToGL(parameters.format, parameters.srgb)), (isHDR || floatData) ? GL_FLOAT : GL_UNSIGNED_BYTE, pixels));
		GLCall(glGenerateMipmap(GL_TEXTURE_2D));
	}

	GLTextureCube::GLTextureCube(uint32_t size)
	{
		PROFILE_FUNCTION();
		numMips = Texture::calculateMipMapCount(size, size);

		GLCall(glGenTextures(1, &handle));
		GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, handle));
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, numMips);
#ifndef PLATFORM_MOBILE
		GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
#endif
		GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
		GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

		for (int32_t i = 0; i < 6; ++i)
		{
			GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, textureFormatToGL(parameters.format, false), size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr));
		}
		GLCall(glGenerateMipmap(GL_TEXTURE_CUBE_MAP));
		width         = size;
		height        = size;
		textureFormat = parameters.format;
	}

	GLTextureCube::GLTextureCube(const std::string &filePath)
	{
		files[0]      = filePath;
		handle        = loadFromFile();
		textureFormat = parameters.format;
	}

	GLTextureCube::GLTextureCube(const std::array<std::string, 6> &initFiles)
	{
		for (uint32_t i = 0; i < 6; i++)
			files[i] = initFiles[i];
		handle        = loadFromFiles();
		textureFormat = parameters.format;
	}

	GLTextureCube::GLTextureCube(const std::vector<std::string> &initFiles, uint32_t mips, const TextureParameters &params, const TextureLoadOptions &loadOptions, const InputFormat &format) :
	    parameters(params), numMips(mips)
	{
		for (uint32_t i = 0; i < mips; i++)
			files[i] = initFiles[i];

		if (format == InputFormat::VERTICAL_CROSS)
			handle = loadFromVCross(mips);

		textureFormat = parameters.format;
	}

	GLTextureCube::GLTextureCube(uint32_t size, TextureFormat format, int32_t numMips) :
	    numMips(numMips)
	{
		PROFILE_FUNCTION();
		GLCall(glGenTextures(1, &handle));
		GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, handle));

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, numMips);

#ifndef PLATFORM_MOBILE
		GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
#endif
		GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
		GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

		auto internalFormat = textureFormatToGL(format, false);
		auto dataFormat     = internalFormatToFormat(internalFormat);

		auto floatData = format == TextureFormat::RGB16 || format == TextureFormat::RGB32 || format == TextureFormat::RGBA16 || format == TextureFormat::RGBA32;

		for (int32_t i = 0; i < 6; ++i)
		{
			//TODO the format could have problem....to be fixed;
			GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, size, size, 0, dataFormat, floatData ? GL_FLOAT : GL_UNSIGNED_BYTE, nullptr));
		}
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		width         = size;
		height        = size;
		textureFormat = format;
	}

	GLTextureCube::~GLTextureCube()
	{
		GLCall(glDeleteTextures(numMips, &handle));
	}

	auto GLTextureCube::bind(uint32_t slot) const -> void
	{
		PROFILE_FUNCTION();
		GLCall(glActiveTexture(GL_TEXTURE0 + slot));
		GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, handle));
	}

	auto GLTextureCube::unbind(uint32_t slot) const -> void
	{
		PROFILE_FUNCTION();
		GLCall(glActiveTexture(GL_TEXTURE0 + slot));
		GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
	}

	auto GLTextureCube::update(CommandBuffer *commandBuffer, FrameBuffer *framebuffer, int32_t cubeIndex, int32_t mipmapLevel /*= 0*/) -> void
	{
		PROFILE_FUNCTION();
	}

	auto GLTextureCube::generateMipmap(const CommandBuffer *commandBuffer) -> void
	{
		PROFILE_FUNCTION();
		GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, handle));
		GLCall(glGenerateMipmap(GL_TEXTURE_CUBE_MAP));
	}

	auto GLTextureCube::loadFromFile() -> uint32_t
	{
		return 0;
	}

	auto GLTextureCube::loadFromFiles() -> uint32_t
	{
		PROFILE_FUNCTION();
		const auto &xPos = files[0];
		const auto &xNeg = files[1];
		const auto &yPos = files[2];
		const auto &yNeg = files[3];
		const auto &zPos = files[4];
		const auto &zNeg = files[5];

		parameters.format = TextureFormat::RGBA8;

		auto xp = ImageLoader::loadAsset(files[0]);
		auto xn = ImageLoader::loadAsset(files[1]);
		auto yp = ImageLoader::loadAsset(files[2]);
		auto yn = ImageLoader::loadAsset(files[3]);
		auto zp = ImageLoader::loadAsset(files[4]);
		auto zn = ImageLoader::loadAsset(files[5]);

		uint32_t width  = xp->getWidth();
		uint32_t height = xp->getHeight();

		uint32_t result;
		GLCall(glGenTextures(1, &result));
		GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, result));
		GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
		GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

		uint32_t internalFormat = textureFormatToGL(parameters.format, parameters.srgb);
		uint32_t format         = internalFormatToFormat(internalFormat);

		GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, xp->getData()));
		GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, xn->getData()));
		GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, yp->getData()));
		GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, yn->getData()));
		GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, zp->getData()));
		GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, zn->getData()));

		GLCall(glGenerateMipmap(GL_TEXTURE_CUBE_MAP));

		return result;
	}

	auto GLTextureCube::loadFromVCross(uint32_t mips) -> uint32_t
	{
		PROFILE_FUNCTION();
		uint32_t   srcWidth        = 0;
		uint32_t   srcHeight       = 0;
		uint32_t   bits            = 0;
		uint8_t ***cubeTextureData = new uint8_t **[mips];
		for (uint32_t i = 0; i < mips; i++)
			cubeTextureData[i] = new uint8_t *[6];

		uint32_t *faceWidths  = new uint32_t[mips];
		uint32_t *faceHeights = new uint32_t[mips];

		for (uint32_t m = 0; m < mips; m++)
		{
			auto           image = ImageLoader::loadAsset(files[m]);
			const uint8_t *data  = (const uint8_t *) image->getData();
			srcWidth             = image->getWidth();
			srcHeight            = image->getHeight();
			parameters.format    = image->getPixelFormat();
			bits                 = image->isHDR() ? 32 : 8;
			uint32_t stride      = bits / 8;

			uint32_t face       = 0;
			uint32_t faceWidth  = srcWidth / 3;
			uint32_t faceHeight = srcHeight / 4;
			faceWidths[m]       = faceWidth;
			faceHeights[m]      = faceHeight;
			for (uint32_t cy = 0; cy < 4; cy++)
			{
				for (uint32_t cx = 0; cx < 3; cx++)
				{
					if (cy == 0 || cy == 2 || cy == 3)
					{
						if (cx != 1)
							continue;
					}

					cubeTextureData[m][face] = new uint8_t[faceWidth * faceHeight * stride];

					for (uint32_t y = 0; y < faceHeight; y++)
					{
						uint32_t offset = y;
						if (face == 5)
							offset = faceHeight - (y + 1);
						uint32_t yp = cy * faceHeight + offset;
						for (uint32_t x = 0; x < faceWidth; x++)
						{
							offset = x;
							if (face == 5)
								offset = faceWidth - (x + 1);
							uint32_t xp = cx * faceWidth + offset;

							cubeTextureData[m][face][(x + y * faceWidth) * stride + 0] = data[(xp + yp * srcWidth) * stride + 0];
							cubeTextureData[m][face][(x + y * faceWidth) * stride + 1] = data[(xp + yp * srcWidth) * stride + 1];
							cubeTextureData[m][face][(x + y * faceWidth) * stride + 2] = data[(xp + yp * srcWidth) * stride + 2];
							if (stride >= 4)
								cubeTextureData[m][face][(x + y * faceWidth) * stride + 3] = data[(xp + yp * srcWidth) * stride + 3];
						}
					}
					face++;
				}
			}
		}

		uint32_t result;
		GLCall(glGenTextures(1, &result));
		GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, result));
		GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
		GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));

		uint32_t internalFormat = textureFormatToGL(parameters.format, parameters.srgb);
		uint32_t format         = internalFormatToFormat(internalFormat);
		for (uint32_t m = 0; m < mips; m++)
		{
			GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, m, internalFormat, faceWidths[m], faceHeights[m], 0, format, GL_UNSIGNED_BYTE, cubeTextureData[m][3]));
			GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, m, internalFormat, faceWidths[m], faceHeights[m], 0, format, GL_UNSIGNED_BYTE, cubeTextureData[m][1]));

			GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, m, internalFormat, faceWidths[m], faceHeights[m], 0, format, GL_UNSIGNED_BYTE, cubeTextureData[m][0]));
			GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, m, internalFormat, faceWidths[m], faceHeights[m], 0, format, GL_UNSIGNED_BYTE, cubeTextureData[m][4]));

			GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, m, internalFormat, faceWidths[m], faceHeights[m], 0, format, GL_UNSIGNED_BYTE, cubeTextureData[m][2]));
			GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, m, internalFormat, faceWidths[m], faceHeights[m], 0, format, GL_UNSIGNED_BYTE, cubeTextureData[m][5]));
		}
		GLCall(glGenerateMipmap(GL_TEXTURE_CUBE_MAP));

		GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));

		for (uint32_t m = 0; m < mips; m++)
		{
			for (uint32_t f = 0; f < 6; f++)
			{
				delete[] cubeTextureData[m][f];
			}
			delete[] cubeTextureData[m];
		}
		delete[] cubeTextureData;
		delete[] faceHeights;
		delete[] faceWidths;

		return result;
	}

	GLTextureDepth::GLTextureDepth(uint32_t width, uint32_t height, bool stencil) :
	    width(width), height(height), stencil(stencil)
	{
		PROFILE_FUNCTION();
		GLCall(glGenTextures(1, &handle));
		format = TextureFormat::DEPTH;
		init();
	}

	GLTextureDepth::~GLTextureDepth()
	{
		GLCall(glDeleteTextures(1, &handle));
	}

	auto GLTextureDepth::bind(uint32_t slot) const -> void
	{
		PROFILE_FUNCTION();
		GLCall(glActiveTexture(GL_TEXTURE0 + slot));
		GLCall(glBindTexture(GL_TEXTURE_2D, handle));
	}

	auto GLTextureDepth::unbind(uint32_t slot) const -> void
	{
		PROFILE_FUNCTION();
		GLCall(glActiveTexture(GL_TEXTURE0 + slot));
		GLCall(glBindTexture(GL_TEXTURE_2D, 0));
	}

	auto GLTextureDepth::resize(uint32_t width, uint32_t height, CommandBuffer *commandBuffer) -> void
	{
		this->width  = width;
		this->height = height;
		this->format = stencil ? TextureFormat::DEPTH_STENCIL : TextureFormat::DEPTH;
		init();
	}

	auto GLTextureDepth::init() -> void
	{
		PROFILE_FUNCTION();
		GLCall(glBindTexture(GL_TEXTURE_2D, handle));
		if (format == TextureFormat::DEPTH_STENCIL)
		{
			GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL));
		}
		else
		{
			GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL));
		}
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL));
#ifndef PLATFORM_MOBILE
		GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE));
#endif
		GLCall(glBindTexture(GL_TEXTURE_2D, 0));
	}

	GLTextureDepthArray::GLTextureDepthArray(uint32_t width, uint32_t height, uint32_t count) :
	    width(width), height(height), count(count)
	{
		format = TextureFormat::DEPTH;
		init();
	}

	GLTextureDepthArray::~GLTextureDepthArray()
	{
		GLCall(glDeleteTextures(1, &handle));
	}

	auto GLTextureDepthArray::bind(uint32_t slot) const -> void
	{
		PROFILE_FUNCTION();
		GLCall(glActiveTexture(GL_TEXTURE0 + slot));
		GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, handle));
	}

	auto GLTextureDepthArray::unbind(uint32_t slot) const -> void
	{
		PROFILE_FUNCTION();
		GLCall(glActiveTexture(GL_TEXTURE0 + slot));
		GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, 0));
	}

	auto GLTextureDepthArray::resize(uint32_t width, uint32_t height, uint32_t count) -> void
	{
		PROFILE_FUNCTION();
		this->width  = width;
		this->height = height;
		this->count  = count;

		GLCall(glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, width, height, count, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr));
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT));
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT));
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
#ifndef PLATFORM_MOBILE
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_NONE));
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY));
#endif
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL));
		GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, 0));
	}

	auto GLTextureDepthArray::init() -> void
	{
		GLCall(glGenTextures(1, &handle));
		GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, handle));

		GLCall(glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, width, height, count, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr));

		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT));
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT));
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
#ifndef PLATFORM_MOBILE
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_NONE));
#endif
		GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL));
		GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, 0));
	}

	GLTexture3D::GLTexture3D(uint32_t width, uint32_t height, uint32_t depth, TextureParameters parameters, TextureLoadOptions loadOptions) :
	    width(width), height(height), depth(depth), parameters(parameters), loadOptions(loadOptions)
	{
		init(width, height, depth);
	}

	GLTexture3D::~GLTexture3D()
	{
		GLCall(glDeleteTextures(1, &handle));
	}

	auto GLTexture3D::init(uint32_t width, uint32_t height, uint32_t depth) -> void
	{
		PROFILE_FUNCTION();
		GLCall(glGenTextures(1, &handle));
		GLCall(glBindTexture(GL_TEXTURE_3D, handle));

		GLCall(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, textureWrapToGL(parameters.wrap)));
		GLCall(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, textureWrapToGL(parameters.wrap)));
		GLCall(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, textureWrapToGL(parameters.wrap)));

		GLCall(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, parameters.magFilter == TextureFilter::Linear? GL_LINEAR : GL_NEAREST));
		
		if (loadOptions.generateMipMaps)
		{
			GLCall(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, parameters.minFilter == TextureFilter::Linear ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST));
		}
		else 
		{
			GLCall(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, parameters.minFilter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST));
		}
/*
* 
* 
		glTexImage3D(GLenum target,
			GLint level,
			GLint internalformat,
			GLsizei width,
			GLsizei height,
			GLsizei depth,
			GLint border,
			GLenum format, //GL_RED, GL_RG, GL_RGB, GL_BGR, GL_RGBA, GL_BGRA, GL_RED_INTEGER, GL_RG_INTEGER, GL_RGB_INTEGER, GL_BGR_INTEGER, GL_RGBA_INTEGER, GL_BGRA_INTEGER, GL_STENCIL_INDEX, GL_DEPTH_COMPONENT, GL_DEPTH_STENCIL.
			GLenum type,
			const void* data);*/

		auto internalFormat = textureFormatToGL(parameters.format, false);
		auto textureFormat = internalFormatToFormat(internalFormat);
		auto dataType = textureDataType(parameters.format);

		GLCall(glTexImage3D(GL_TEXTURE_3D, 0, internalFormat, width, height, depth, 0, textureFormat, dataType, NULL));
		if (loadOptions.generateMipMaps)
		{
			GLCall(glGenerateMipmap(GL_TEXTURE_3D));
		}
		GLCall(glBindImageTexture(0, handle, 0, GL_FALSE, 0, GL_READ_WRITE, internalFormat));
	}

	auto GLTexture3D::bind(uint32_t slot) const -> void
	{
		PROFILE_FUNCTION();
		GLCall(glActiveTexture(GL_TEXTURE0 + slot));
		GLCall(glBindTexture(GL_TEXTURE_3D, handle));
	}

	auto GLTexture3D::unbind(uint32_t slot) const -> void
	{
		PROFILE_FUNCTION();
		GLCall(glActiveTexture(GL_TEXTURE0 + slot));
		GLCall(glBindTexture(GL_TEXTURE_3D, handle));
	}

	auto GLTexture3D::generateMipmaps() -> void
	{
		PROFILE_FUNCTION();
		GLCall(glBindTexture(GL_TEXTURE_3D, handle));
		GLCall(glGenerateMipmap(GL_TEXTURE_3D));
		GLCall(glBindTexture(GL_TEXTURE_3D, 0));
	}

	auto GLTexture3D::bindImageTexture(uint32_t unit, bool read, bool write, uint32_t level, uint32_t layer) -> void
	{
		PROFILE_FUNCTION();
		auto flag = read && write ? GL_READ_WRITE :
		            read          ? GL_READ_ONLY :
		            write         ? GL_WRITE_ONLY :
                                    GL_READ_WRITE;
		bind(unit);
		GLCall(glBindImageTexture(unit, handle, level, true, layer, flag, textureFormatToGL(parameters.format, false)));
	}

	auto GLTexture3D::buildTexture3D(TextureFormat format, uint32_t width, uint32_t height, uint32_t depth) -> void
	{
		PROFILE_FUNCTION();
		GLCall(glBindTexture(GL_TEXTURE_3D, handle));

		GLCall(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, textureWrapToGL(parameters.wrap)));
		GLCall(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, textureWrapToGL(parameters.wrap)));
		GLCall(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, textureWrapToGL(parameters.wrap)));

		GLCall(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, parameters.magFilter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST));
		if (loadOptions.generateMipMaps)
		{
			GLCall(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, parameters.minFilter == TextureFilter::Linear ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST));
		}
		else 
		{
			GLCall(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, parameters.minFilter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST));
		}

		auto internalFormat = textureFormatToGL(format, false);
		auto textureFormat = internalFormatToFormat(internalFormat);
		auto dataType = textureDataType(format);

		GLCall(glTexImage3D(GL_TEXTURE_3D, 0, internalFormat, width, height, depth, 0, textureFormat, dataType, NULL));
		if (loadOptions.generateMipMaps) 
		{
			GLCall(glGenerateMipmap(GL_TEXTURE_3D));
		}
		GLCall(glBindTexture(GL_TEXTURE_3D, 0));
	}

	auto GLTexture3D::clear() -> void
	{
		PROFILE_FUNCTION();
		std::vector<uint8_t> emptyData(width * height * depth * textureFormatSize(parameters.format), 0);//float and int is 4.

		auto internalFormat = internalFormatToFormat(textureFormatToGL(parameters.format, false));
		auto dataType = textureDataType(parameters.format);

		glBindTexture(GL_TEXTURE_3D, handle);
		glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, width, height, depth, internalFormat, dataType, &emptyData[0]);
		glBindTexture(GL_TEXTURE_3D, 0);
	}
}        // namespace maple
