#pragma once
#include "RHI/Texture.h"
#include <array>

namespace maple
{
	class GLTexture2D : public Texture2D
	{
	  public:
		GLTexture2D(uint32_t width, uint32_t height, const void *data, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions());
		GLTexture2D(const std::string &name, const std::string &fileName, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions());
		GLTexture2D();
		~GLTexture2D();

		auto bind(uint32_t slot) const -> void override;
		auto unbind(uint32_t slot) const -> void override;
		auto buildTexture(TextureFormat internalformat, uint32_t width, uint32_t height, bool srgb, bool depth, bool samplerShadow, bool mipmap, bool image, uint32_t accessFlag) -> void override;
		auto bindImageTexture(uint32_t unit, bool read = false, bool write = false, uint32_t level = 0, uint32_t layer = 0) -> void override;
		auto update(int32_t x, int32_t y, int32_t w, int32_t h, const void *buffer) -> void override;

		auto setData(const void *pixels) -> void;

		auto getHandle() const -> void * override
		{
			return (void *) (size_t) handle;
		}

		inline auto getWidth() const -> uint32_t override
		{
			return width;
		}

		inline auto getHeight() const -> uint32_t override
		{
			return height;
		}

		inline auto getFilePath() const -> const std::string & override
		{
			return fileName;
		}

		inline auto getType() const -> TextureType override
		{
			return TextureType::Color;
		}

		inline auto getFormat() const -> TextureFormat override
		{
			return format;
		}

		inline auto getPath() const -> std::string override
		{
			return fileName;
		}

	  private:
		auto               load(const void *data) -> void;
		bool               isHDR  = false;
		uint32_t           handle = 0;
		uint32_t           width  = 0;
		uint32_t           height = 0;
		std::string        fileName;
		TextureFormat      format;
		TextureParameters  parameters;
		TextureLoadOptions loadOptions;
		bool               storedImage = false;
	};

	class GLTexture3D : public Texture3D
	{
	  public:
		GLTexture3D(uint32_t width, uint32_t height, uint32_t depth, TextureParameters parameters, TextureLoadOptions loadOptions);
		~GLTexture3D();

		auto init(uint32_t width, uint32_t height, uint32_t depth) -> void;

		auto bind(uint32_t slot = 0) const -> void override;

		auto unbind(uint32_t slot = 0) const -> void override;

		auto generateMipmaps() -> void override;

		auto bindImageTexture(uint32_t unit, bool read, bool write, uint32_t level, uint32_t layer) -> void override;

		auto buildTexture3D(TextureFormat format, uint32_t width, uint32_t height, uint32_t depth) -> void override;

		virtual auto getFilePath() const -> const std::string & override
		{
			return filePath;
		};

		auto getHandle() const -> void * override
		{
			return (void *) (size_t) handle;
		}

		inline auto getWidth() const -> uint32_t override
		{
			return width;
		}

		inline auto getHeight() const -> uint32_t override
		{
			return height;
		}

		inline auto getDepth() const -> uint32_t
		{
			return height;
		}
		inline auto getFormat() const -> TextureFormat override
		{
			return parameters.format;
		}

		auto clear() -> void override;

	  private:
		uint32_t      handle = 0;
		uint32_t      width  = 0;
		uint32_t      height = 0;
		uint32_t      depth  = 0;
		TextureParameters parameters;
		TextureLoadOptions loadOptions;
		std::string   filePath;
	};

	class GLTextureCube : public TextureCube
	{
	  public:
		GLTextureCube(uint32_t size);
		GLTextureCube(uint32_t size, TextureFormat format, int32_t numMips);
		GLTextureCube(const std::string &filePath);
		GLTextureCube(const std::array<std::string, 6> &files);
		GLTextureCube(const std::vector<std::string> &files, uint32_t mips, const TextureParameters &params, const TextureLoadOptions &loadOptions, const InputFormat &format);
		~GLTextureCube();

		inline auto getHandle() const -> void * override
		{
			return (void *) (size_t) handle;
		}

		auto bind(uint32_t slot = 0) const -> void override;
		auto unbind(uint32_t slot = 0) const -> void override;

		inline auto getMipMapLevels() const -> uint32_t override
		{
			return numMips;
		}

		inline auto getSize() const -> uint32_t override
		{
			return size;
		}

		inline auto getFilePath() const -> const std::string & override
		{
			return files[0];
		}

		inline auto getWidth() const -> uint32_t override
		{
			return width;
		}

		inline auto getHeight() const -> uint32_t override
		{
			return height;
		}

		inline auto getType() const -> TextureType override
		{
			return TextureType::Cube;
		}

		inline auto getFormat() const -> TextureFormat override
		{
			return textureFormat;
		}

		auto update(CommandBuffer *commandBuffer, FrameBuffer *framebuffer, int32_t cubeIndex, int32_t mipmapLevel = 0) -> void override;

		auto generateMipmap(const CommandBuffer *commandBuffer) -> void override;

	  private:
		static auto loadFromFile() -> uint32_t;
		auto        loadFromFiles() -> uint32_t;
		auto        loadFromVCross(uint32_t mips) -> uint32_t;

		uint32_t handle  = 0;
		uint32_t width   = 0;
		uint32_t height  = 0;
		uint32_t size    = 0;
		uint32_t bits    = 0;
		uint32_t numMips = 0;

		std::string        files[MAX_MIPS];
		InputFormat        inputFormat;
		TextureFormat      textureFormat;
		TextureParameters  parameters;
		TextureLoadOptions loadOptions;
	};

	class GLTextureDepth : public TextureDepth
	{
	  public:
		GLTextureDepth(uint32_t width, uint32_t height, bool stencil = false);
		~GLTextureDepth();

		auto bind(uint32_t slot = 0) const -> void override;
		auto unbind(uint32_t slot = 0) const -> void override;
		auto resize(uint32_t width, uint32_t height, CommandBuffer *commandBuffer) -> void override;

		inline auto getHandle() const -> void * override
		{
			return (void *) (size_t) handle;
		}

		inline auto getFilePath() const -> const std::string & override
		{
			return name;
		}

		inline auto getWidth() const -> uint32_t override
		{
			return width;
		}

		inline auto getHeight() const -> uint32_t override
		{
			return height;
		}

		inline auto getType() const -> TextureType override
		{
			return TextureType::Depth;
		}

		inline auto getFormat() const -> TextureFormat override
		{
			return format;
		}

	  protected:
		auto init() -> void;

		bool     stencil = false;
		uint32_t handle  = 0;
		uint32_t width   = 0;
		uint32_t height  = 0;

		TextureFormat format;
	};

	class GLTextureDepthArray : public TextureDepthArray
	{
	  public:
		GLTextureDepthArray(uint32_t width, uint32_t height, uint32_t count);
		~GLTextureDepthArray();

		auto bind(uint32_t slot = 0) const -> void override;
		auto unbind(uint32_t slot = 0) const -> void override;
		auto resize(uint32_t width, uint32_t height, uint32_t count) -> void override;
		auto init() -> void override;

		inline auto getHandle() const -> void * override
		{
			return (void *) (size_t) handle;
		}

		inline auto getFilePath() const -> const std::string & override
		{
			return name;
		}

		inline auto getWidth() const -> uint32_t override
		{
			return width;
		}

		inline auto getHeight() const -> uint32_t override
		{
			return height;
		}

		inline auto setCount(uint32_t count)
		{
			this->count = count;
		}

		inline auto getType() const -> TextureType override
		{
			return TextureType::DepthArray;
		}

		inline auto getFormat() const -> TextureFormat override
		{
			return format;
		}

		inline auto getCount() const -> uint32_t
		{
			return count;
		}

	  private:
		uint32_t      handle = 0;
		uint32_t      width  = 0;
		uint32_t      height = 0;
		uint32_t      count  = 0;
		TextureFormat format;
	};
}        // namespace maple
