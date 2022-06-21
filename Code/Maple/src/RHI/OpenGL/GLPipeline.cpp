//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "GLPipeline.h"
#include "Application.h"
#include "Engine/Profiler.h"
#include "GLDescriptorSet.h"
#include "GLFramebuffer.h"
#include "GLRenderDevice.h"
#include "GLRenderPass.h"
#include "GLShader.h"
#include "GLSwapChain.h"

namespace maple
{
	namespace
	{
		inline auto vertexAtrribPointer(Format format, uint32_t index, size_t offset, uint32_t stride)
		{
			switch (format)
			{
				case Format::R32_FLOAT:
					GLCall(glVertexAttribPointer(index, 1, GL_FLOAT, false, stride, (const void *) (intptr_t)(offset)));
					break;
				case Format::R32G32_FLOAT:
					GLCall(glVertexAttribPointer(index, 2, GL_FLOAT, false, stride, (const void *) (intptr_t)(offset)));
					break;
				case Format::R32G32B32_FLOAT:
					GLCall(glVertexAttribPointer(index, 3, GL_FLOAT, false, stride, (const void *) (intptr_t)(offset)));
					break;
				case Format::R32G32B32A32_FLOAT:
					GLCall(glVertexAttribPointer(index, 4, GL_FLOAT, false, stride, (const void *) (intptr_t)(offset)));
					break;
				case Format::R8_UINT:
					GLCall(glVertexAttribPointer(index, 1, GL_UNSIGNED_BYTE, false, stride, (const void *) (intptr_t)(offset)));
					break;
				case Format::R32_UINT:
					GLCall(glVertexAttribPointer(index, 1, GL_UNSIGNED_INT, false, stride, (const void *) (intptr_t)(offset)));
					break;
				case Format::R32G32_UINT:
					GLCall(glVertexAttribPointer(index, 2, GL_UNSIGNED_INT, false, stride, (const void *) (intptr_t)(offset)));
					break;
				case Format::R32G32B32_UINT:
					GLCall(glVertexAttribPointer(index, 3, GL_UNSIGNED_INT, false, stride, (const void *) (intptr_t)(offset)));
					break;
				case Format::R32G32B32A32_UINT:
					GLCall(glVertexAttribPointer(index, 4, GL_UNSIGNED_INT, false, stride, (const void *) (intptr_t)(offset)));
					break;
				case Format::R32G32_INT:
					GLCall(glVertexAttribPointer(index, 2, GL_INT, false, stride, (const void *) (intptr_t)(offset)));
					break;
				case Format::R32G32B32_INT:
					GLCall(glVertexAttribPointer(index, 3, GL_INT, false, stride, (const void *) (intptr_t)(offset)));
					break;
				case Format::R32G32B32A32_INT:
					GLCall(glVertexAttribPointer(index, 4, GL_INT, false, stride, (const void *) (intptr_t)(offset)));
					break;
			}
		}
	}        // namespace

	GLPipeline::GLPipeline(const PipelineInfo &pipelineDesc)
	{
		init(pipelineDesc);
	}

	GLPipeline::~GLPipeline()
	{
		PROFILE_FUNCTION();
		if (!shader->isComputeShader())
			glDeleteVertexArrays(1, &vertexArray);
	}

	auto GLPipeline::init(const PipelineInfo &pipelineDesc) -> bool
	{
		PROFILE_FUNCTION();
		transparencyEnabled = pipelineDesc.transparencyEnabled;
		description         = pipelineDesc;
		shader              = description.shader;
		if (!shader->isComputeShader())
		{
			GLCall(glGenVertexArrays(1, &vertexArray));
			createFrameBuffers();
		}
		return true;
	}

	auto GLPipeline::bindVertexArray() -> void
	{
		PROFILE_FUNCTION();
		GLCall(glBindVertexArray(vertexArray));

		auto &   bufferLayout = std::static_pointer_cast<GLShader>(shader)->getLayout();
		auto &   vertexLayout = bufferLayout.getLayout();
		uint32_t count        = 0;

		for (auto &layout : vertexLayout)
		{
			GLCall(glEnableVertexAttribArray(count));
			size_t offset = static_cast<size_t>(layout.offset);
			vertexAtrribPointer(layout.format, count, offset, bufferLayout.getStride());
			count++;
		}
	}

	auto GLPipeline::createFrameBuffers() -> void
	{
		PROFILE_FUNCTION();
		std::vector<std::shared_ptr<Texture>> attachments;

		if (description.swapChainTarget)
		{
			attachments.emplace_back(Application::getGraphicsContext()->getSwapChain()->getImage(0));
		}
		else
		{
			for (auto texture : description.colorTargets)
			{
				if (texture)
				{
					attachments.emplace_back(texture);
				}
			}
		}

		if (description.depthTarget)
		{
			attachments.emplace_back(description.depthTarget);
		}

		if (description.depthArrayTarget)
		{
			attachments.emplace_back(description.depthArrayTarget);
		}

		RenderPassInfo renderPassDesc;
		renderPassDesc.attachments = attachments;
		renderPassDesc.clear       = description.clearTargets;

		renderPass = RenderPass::create(renderPassDesc);

		FrameBufferInfo frameBufferDesc{};
		frameBufferDesc.width      = getWidth();
		frameBufferDesc.height     = getHeight();
		frameBufferDesc.renderPass = renderPass;

		if (description.swapChainTarget)
		{
			auto swapChain = Application::getGraphicsContext()->getSwapChain();
			for (uint32_t i = 0; i < swapChain->getSwapChainBufferCount(); i++)
			{
				frameBufferDesc.screenFBO   = true;
				attachments[0]              = swapChain->getImage(i);
				frameBufferDesc.attachments = attachments;

				frameBuffers.emplace_back(FrameBuffer::create(frameBufferDesc));
			}
		}
		else if (description.depthArrayTarget)
		{
			for (uint32_t i = 0; i < std::static_pointer_cast<GLTextureDepthArray>(description.depthArrayTarget)->getCount(); ++i)
			{
				frameBufferDesc.layer     = i;
				frameBufferDesc.screenFBO = false;

				attachments[0]              = description.depthArrayTarget;
				frameBufferDesc.attachments = attachments;

				frameBuffers.emplace_back(FrameBuffer::create(frameBufferDesc));
			}
		}
		else
		{
			frameBufferDesc.attachments = attachments;
			frameBufferDesc.screenFBO   = false;
			frameBuffers.emplace_back(FrameBuffer::create(frameBufferDesc));
		}
	}

	auto GLPipeline::getWidth() -> uint32_t
	{
		if (description.swapChainTarget)
		{
			return std::static_pointer_cast<GLSwapChain>(Application::getGraphicsContext()->getSwapChain())->getWidth();
		}
		if (description.colorTargets[0])
			return description.colorTargets[0]->getWidth();

		if (description.depthTarget)
			return description.depthTarget->getWidth();

		if (description.depthArrayTarget)
			return description.depthArrayTarget->getWidth();

		LOGW("Invalid pipeline width");

		return 0;
	}

	auto GLPipeline::getHeight() -> uint32_t
	{
		if (description.swapChainTarget)
		{
			return std::static_pointer_cast<GLSwapChain>(Application::getGraphicsContext()->getSwapChain())->getHeight();
		}
		if (description.colorTargets[0])
			return description.colorTargets[0]->getHeight();

		if (description.depthTarget)
			return description.depthTarget->getHeight();

		if (description.depthArrayTarget)
			return description.depthArrayTarget->getHeight();

		LOGW("Invalid pipeline height");

		return 0;
	}

	auto GLPipeline::bind(const CommandBuffer *commandBuffer, uint32_t layer, int32_t cubeFace, int32_t mipMapLevel) -> FrameBuffer *
	{
		PROFILE_FUNCTION();
		if (!shader->isComputeShader())
		{
			std::shared_ptr<FrameBuffer> frameBuffer;

			if (description.swapChainTarget)
			{
				auto swapChain = Application::getGraphicsContext()->getSwapChain();
				frameBuffer    = frameBuffers[swapChain->getCurrentBufferIndex()];
			}
			else if (description.depthArrayTarget)
			{
				frameBuffer = frameBuffers[layer];
			}
			else
			{
				frameBuffer = frameBuffers[0];
			}

			auto mipScale = std::pow(0.5, mipMapLevel);

			renderPass->beginRenderPass(commandBuffer, description.clearColor, frameBuffer.get(), SubPassContents::Inline, getWidth() * mipScale, getHeight() * mipScale, cubeFace, mipMapLevel);
			description.shader->bind();

			RenderDevice::setStencilTest(description.stencilTest);
			RenderDevice::setDepthTest(description.depthTest);
			//RenderDevice::setStencilMask(0xFF);

			if (description.stencilTest)
			{
				RenderDevice::setStencilOp(description.stencilFail, description.stencilDepthFail, description.stencilDepthPass);
				RenderDevice::setStencilFunction(description.stencilFunc, 1, 0xff);
				RenderDevice::setStencilMask(description.stencilMask);
			}
			else
			{
				RenderDevice::setStencilMask(0xFF);
				RenderDevice::setStencilOp(StencilType::Keep, StencilType::Keep, StencilType::Keep);
			}

			if (transparencyEnabled)
			{
				glEnable(GL_BLEND);

				GLCall(glBlendEquation(GL_FUNC_ADD));

				if (description.blendMode == BlendMode::SrcAlphaOneMinusSrcAlpha)
				{
					GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
				}
				else if (description.blendMode == BlendMode::ZeroSrcColor)
				{
					GLCall(glBlendFunc(GL_ZERO, GL_SRC_COLOR));
				}
				else if (description.blendMode == BlendMode::OneZero)
				{
					GLCall(glBlendFunc(GL_ONE, GL_ZERO));
				}
				else
				{
					GLCall(glBlendFunc(GL_NONE, GL_NONE));
				}
			}
			else
				glDisable(GL_BLEND);

			glEnable(GL_CULL_FACE);

			switch (description.cullMode)
			{
				case CullMode::Back:
					glCullFace(GL_BACK);
					break;
				case CullMode::Front:
					glCullFace(GL_FRONT);
					break;
				case CullMode::FrontAndBack:
					glCullFace(GL_FRONT_AND_BACK);
					break;
				case CullMode::None:
					glDisable(GL_CULL_FACE);
					break;
			}

			GLCall(glFrontFace(GL_CCW));

			return frameBuffer.get();
		}
		return nullptr;
	}

	auto GLPipeline::end(const CommandBuffer *commandBuffer) -> void
	{
		PROFILE_FUNCTION();
		if (!shader->isComputeShader())
		{
			renderPass->endRenderPass(commandBuffer);
		}
	}

	auto GLPipeline::clearRenderTargets(const CommandBuffer *commandBuffer) -> void
	{
		PROFILE_FUNCTION();
		for (auto framebuffer : frameBuffers)
		{
			framebuffer->bind();
			GLRenderDevice::clear(RendererBufferColor | RendererBufferDepth | RendererBufferStencil);
		}
	}
}        // namespace maple
