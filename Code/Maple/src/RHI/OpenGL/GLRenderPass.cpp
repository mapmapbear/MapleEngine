//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "GLRenderPass.h"
#include "GLFrameBuffer.h"
#include "GLRenderDevice.h"

namespace maple
{
	GLRenderPass::GLRenderPass(const RenderPassInfo &renderPassDesc)
	{
		clear = renderPassDesc.clear;
	}

	GLRenderPass::~GLRenderPass()
	{
	}

	auto GLRenderPass::beginRenderPass(const CommandBuffer *commandBuffer, const glm::vec4 &clearColor, FrameBuffer *frame, SubPassContents contents, uint32_t width, uint32_t height, int32_t cubeFace, int32_t mipMapLevel) const -> void
	{
		GLCall(glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w));
		if (frame != nullptr)
		{
			frame->bind(width, height);
			if (cubeFace != -1)
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + cubeFace, (size_t) frame->getColorAttachment(1)->getHandle(), mipMapLevel);
			}
		}
		else
		{
			GLCall(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
			GLCall(glViewport(0, 0, width, height));
		}

		if (clear)
			GLRenderDevice::clear(RendererBufferColor | RendererBufferDepth | RendererBufferStencil);
	}

	auto GLRenderPass::endRenderPass(const CommandBuffer *commandBuffer) -> void
	{
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	}
}        // namespace maple
