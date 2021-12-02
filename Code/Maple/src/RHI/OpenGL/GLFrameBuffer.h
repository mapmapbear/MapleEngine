#pragma once
#include "GLTexture.h"
#include "RHI/FrameBuffer.h"
#include "RHI/OpenGL/GL.h"

namespace maple
{
	enum class Format;

	class MAPLE_EXPORT GLFrameBuffer : public FrameBuffer
	{
	  public:
		GLFrameBuffer();
		GLFrameBuffer(const FrameBufferInfo &frameBufferDesc);
		~GLFrameBuffer();

		inline auto getFrameBuffer() const
		{
			return handle;
		}

		inline auto setClearColor(const glm::vec4 &color) -> void override
		{
			clearColor = color;
		}

		inline auto clear() -> void override
		{}

		inline auto getWidth() const -> uint32_t override
		{
			return width;
		}

		inline auto getHeight() const -> uint32_t override
		{
			return height;
		}

		auto generateFramebuffer() -> void override;
		auto bind(uint32_t width, uint32_t height) const -> void override;
		auto bind() const -> void override;
		auto unbind() const -> void override;
		auto addTextureAttachment(TextureFormat format, const std::shared_ptr<Texture> &texture) -> void override;
		auto addCubeTextureAttachment(TextureFormat format, CubeFace face, const std::shared_ptr<TextureCube> &texture) -> void override;
		auto addShadowAttachment(const std::shared_ptr<Texture> &texture) -> void override;
		auto addTextureLayer(int32_t index, const std::shared_ptr<Texture> &texture) -> void override;
		auto validate() -> void override;

		auto getAttachmentType(TextureFormat format) -> GLenum;

		inline auto &getAttachments() const
		{
			return attachments;
		}

		inline auto getColorAttachment(int32_t id = 0) const -> std::shared_ptr<Texture> override
		{
			return attachments[id];
		}

	  private:
		uint32_t  handle               = 0;
		uint32_t  width                = 0;
		uint32_t  height               = 0;
		uint32_t  colorAttachmentCount = 0;
		bool      screenFrameBuffer    = false;
		glm::vec4 clearColor           = {
            0.5,
            0.5,
            0.5,
            1.f};
		std::vector<GLenum> attachmentData;

		std::vector<std::shared_ptr<Texture>> attachments;
	};
}        // namespace maple
