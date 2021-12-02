#include "GLFrameBuffer.h"
#include "Engine/Profiler.h"
#include "GL.h"
#include "Others/Console.h"
#include "RHI/Texture.h"

namespace maple
{
	GLFrameBuffer::GLFrameBuffer()
	{
		PROFILE_FUNCTION();
		GLCall(glGenFramebuffers(1, &handle));
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, handle));
		colorAttachmentCount = 0;
	}

	GLFrameBuffer::GLFrameBuffer(const FrameBufferInfo &frameBufferDesc)
	{
		PROFILE_FUNCTION();

		attachments          = frameBufferDesc.attachments;
		screenFrameBuffer    = frameBufferDesc.screenFBO;
		width                = frameBufferDesc.width;
		height               = frameBufferDesc.height;
		colorAttachmentCount = 0;

		if (screenFrameBuffer)
		{
			GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
		}
		else
		{
			GLCall(glGenFramebuffers(1, &handle));
			GLCall(glBindFramebuffer(GL_FRAMEBUFFER, handle));

			for (uint32_t i = 0; i < frameBufferDesc.attachments.size(); i++)
			{
				auto &attachment = frameBufferDesc.attachments[i];

				switch (attachment->getType())
				{
					case TextureType::Color:
						addTextureAttachment(attachment->getFormat(), frameBufferDesc.attachments[i]);
						break;
					case TextureType::Depth:
						addTextureAttachment(TextureFormat::DEPTH, frameBufferDesc.attachments[i]);
						break;
					case TextureType::DepthArray:
						addTextureLayer(frameBufferDesc.layer, frameBufferDesc.attachments[i]);
						break;
					case TextureType::Other:
						break;
					case TextureType::Cube:
						//for (auto i = 0; i < 6; i++)
						//{
						//	addCubeTextureAttachment(attachment->getFormat(), static_cast<CubeFace>(i), std::static_pointer_cast<TextureCube>(attachment));
						//}
						break;
				}
			}

			GLCall(glDrawBuffers(static_cast<GLsizei>(attachmentData.size()), attachmentData.data()));
			validate();
			unbind();
		}
	}

	GLFrameBuffer::~GLFrameBuffer()
	{
		PROFILE_FUNCTION();
		if (!screenFrameBuffer)
			GLCall(glDeleteFramebuffers(1, &handle));
	}
	auto GLFrameBuffer::generateFramebuffer() -> void
	{
		PROFILE_FUNCTION();
		if (!screenFrameBuffer)
			GLCall(glGenFramebuffers(1, &handle));
	}
	auto GLFrameBuffer::bind(uint32_t width, uint32_t height) const -> void
	{
		PROFILE_FUNCTION();
		GLCall(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, screenFrameBuffer ? 0 : handle));
		GLCall(glViewport(0, 0, width, height));

		if (!screenFrameBuffer)
			GLCall(glDrawBuffers(static_cast<GLsizei>(attachmentData.size()), attachmentData.data()));
	}

	auto GLFrameBuffer::bind() const -> void
	{
		PROFILE_FUNCTION();
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, screenFrameBuffer ? 0 : handle));
	}

	auto GLFrameBuffer::unbind() const -> void
	{
		PROFILE_FUNCTION();
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	}

	auto GLFrameBuffer::getAttachmentType(TextureFormat format) -> GLenum
	{
		PROFILE_FUNCTION();
		if (Texture::isDepthStencilFormat(format))
		{
			return GL_DEPTH_STENCIL_ATTACHMENT;
		}

		if (Texture::isStencilFormat(format))
		{
			return GL_STENCIL_ATTACHMENT;
		}

		if (Texture::isDepthFormat(format))
		{
			return GL_DEPTH_ATTACHMENT;
		}

		GLenum value = GL_COLOR_ATTACHMENT0 + colorAttachmentCount;
		colorAttachmentCount++;
		return value;
	}

	auto GLFrameBuffer::addTextureAttachment(TextureFormat format, const std::shared_ptr<Texture> &texture) -> void
	{
		PROFILE_FUNCTION();
		auto attachment = getAttachmentType(format);

		if (attachment != GL_DEPTH_ATTACHMENT && attachment != GL_STENCIL_ATTACHMENT && attachment != GL_DEPTH_STENCIL_ATTACHMENT)
		{
			attachmentData.emplace_back(attachment);
		}
		auto id = (GLuint) (size_t) texture->getHandle();
		GLCall(glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, attachment, GL_TEXTURE_2D, id, 0));
	}

	auto GLFrameBuffer::addCubeTextureAttachment(TextureFormat format, CubeFace face, const std::shared_ptr<TextureCube> &texture) -> void
	{
		PROFILE_FUNCTION();
		uint32_t faceID = 0;

		GLenum attachment = getAttachmentType(format);
		if (attachment != GL_DEPTH_ATTACHMENT && attachment != GL_STENCIL_ATTACHMENT && attachment != GL_DEPTH_STENCIL_ATTACHMENT)
		{
			attachmentData.emplace_back(attachment);
		}

		switch (face)
		{
			case CubeFace::PositiveX:
				faceID = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
				break;
			case CubeFace::NegativeX:
				faceID = GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
				break;
			case CubeFace::PositiveY:
				faceID = GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
				break;
			case CubeFace::NegativeY:
				faceID = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
				break;
			case CubeFace::PositiveZ:
				faceID = GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
				break;
			case CubeFace::NegativeZ:
				faceID = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
				break;
		}

		GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, faceID, (GLuint) (size_t) texture->getHandle(), 0));
	}
	auto GLFrameBuffer::addShadowAttachment(const std::shared_ptr<Texture> &texture) -> void
	{
		PROFILE_FUNCTION();
#ifdef PLATFORM_MOBILE
		GLCall(glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, (GLuint) texture->getHandle(), 0, 0));
#else
		GLCall(glFramebufferTextureLayer(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT, (GLuint) (size_t) texture->getHandle(), 0, 0));
#endif
		GLCall(glDrawBuffers(0, GL_NONE));
	}
	auto GLFrameBuffer::addTextureLayer(int32_t index, const std::shared_ptr<Texture> &texture) -> void
	{
		PROFILE_FUNCTION();
#ifdef PLATFORM_MOBILE
		GLCall(glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, (GLuint) (size_t) texture->getHandle(), 0, index));
#else
		GLCall(glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, (GLuint) (size_t) texture->getHandle(), 0, index));
#endif
	}
	auto GLFrameBuffer::validate() -> void
	{
		PROFILE_FUNCTION();
		uint32_t status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			LOGC("create FrameBuffer error. error code : {0:x}", status);
			MAPLE_ASSERT(false, "create FrameBuffer error.");
		}
	}
}        // namespace maple
