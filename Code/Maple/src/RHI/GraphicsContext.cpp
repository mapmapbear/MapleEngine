//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "Others/HashCode.h"
#include "RHI/ImGui/GLImGuiRenderer.h"
#include "RHI/OpenGL/GLCommandBuffer.h"
#include "RHI/OpenGL/GLContext.h"
#include "RHI/OpenGL/GLDescriptorSet.h"
#include "RHI/OpenGL/GLFrameBuffer.h"
#include "RHI/OpenGL/GLIndexBuffer.h"
#include "RHI/OpenGL/GLPipeline.h"
#include "RHI/OpenGL/GLRenderPass.h"
#include "RHI/OpenGL/GLShader.h"
#include "RHI/OpenGL/GLSwapChain.h"
#include "RHI/OpenGL/GLUniformBuffer.h"
#include "RHI/OpenGL/GLVertexBuffer.h"

#include "Application.h"

namespace maple
{
	auto GraphicsContext::create() -> std::shared_ptr<GraphicsContext>
	{
		return std::make_shared<GLContext>();
	}

	auto GraphicsContext::clearUnused() -> void
	{
		for (auto iter = pipelineCache.begin(); iter != pipelineCache.end();)
		{
			if (iter->second.use_count() == 1)
			{
				iter = pipelineCache.erase(iter);
				continue;
			}
			iter++;
		}

		for (auto iter = frameBufferCache.begin(); iter != frameBufferCache.end();)
		{
			if (iter->second.use_count() == 1)
			{
				iter = frameBufferCache.erase(iter);
				continue;
			}
			iter++;
		}
	}

	auto SwapChain::create(uint32_t width, uint32_t height) -> std::shared_ptr<SwapChain>
	{
		return std::make_shared<GLSwapChain>(width, height);
	}

	auto Shader::create(const std::string &filePath) -> std::shared_ptr<Shader>
	{
		return Application::getCache()->emplace<GLShader>(filePath);
	}

	auto Shader::create(const std::vector<uint32_t> &vertData, const std::vector<uint32_t> &fragData) -> std::shared_ptr<Shader>
	{
		return Application::getCache()->emplace<GLShader>(vertData, fragData);
	}

	auto FrameBuffer::create(const FrameBufferInfo &desc) -> std::shared_ptr<FrameBuffer>
	{
		size_t hash = 0;
		HashCode::hashCode(hash, desc.attachments.size(), desc.width, desc.height, desc.layer, desc.renderPass, desc.screenFBO);

		for (uint32_t i = 0; i < desc.attachments.size(); i++)
		{
			HashCode::hashCode(hash, desc.attachments[i]);
#ifdef MAPLE_VULKAN
			VkDescriptorImageInfo *imageHandle = (VkDescriptorImageInfo *) (desc.attachments[i]->getDescriptorInfo());
			HashCode::hashCode(hash, imageHandle->imageLayout, imageHandle->imageView, imageHandle->sampler);
#endif
		}
		auto &frameBufferCache = Application::getGraphicsContext()->getFrameBufferCache();
		auto  found            = frameBufferCache.find(hash);
		if (found != frameBufferCache.end() && found->second)
		{
			return found->second;
		}

		return frameBufferCache.emplace(hash, std::make_shared<GLFrameBuffer>(desc)).first->second;
	}

	auto DescriptorSet::create(const DescriptorInfo &desc) -> std::shared_ptr<DescriptorSet>
	{
		return std::make_shared<GLDescriptorSet>(desc);
	}

	auto CommandBuffer::create() -> std::shared_ptr<CommandBuffer>
	{
		return std::make_shared<GLCommandBuffer>();
	}

	auto ImGuiRenderer::create(uint32_t width, uint32_t height, bool clearScreen) -> std::shared_ptr<ImGuiRenderer>
	{
		return std::make_shared<GLImGuiRenderer>(width, height, clearScreen);
	}

	auto IndexBuffer::create(const uint16_t *data, uint32_t count, BufferUsage bufferUsage) -> std::shared_ptr<IndexBuffer>
	{
		return std::make_shared<GLIndexBuffer>(data, count, bufferUsage);
	}
	auto IndexBuffer::create(const uint32_t *data, uint32_t count, BufferUsage bufferUsage) -> std::shared_ptr<IndexBuffer>
	{
		return std::make_shared<GLIndexBuffer>(data, count, bufferUsage);
	}

	auto Pipeline::get(const PipelineInfo &desc) -> std::shared_ptr<Pipeline>
	{
		size_t hash = 0;

		HashCode::hashCode(hash, desc.shader, desc.cullMode, desc.depthBiasEnabled, desc.drawType, desc.polygonMode, desc.transparencyEnabled);
		HashCode::hashCode(hash, desc.stencilMask, desc.stencilFunc, desc.stencilFail, desc.stencilDepthFail, desc.stencilDepthPass, desc.depthTest);

		for (auto texture : desc.colorTargets)
		{
			if (texture)
			{
				HashCode::hashCode(hash, texture);
				HashCode::hashCode(hash, texture->getWidth(), texture->getHeight());
				HashCode::hashCode(hash, texture->getHandle());
				HashCode::hashCode(hash, texture->getFormat());
#ifdef MAPLE_VULKAN
				VkDescriptorImageInfo *imageHandle = (VkDescriptorImageInfo *) (texture->getDescriptorInfo());
				HashCode::hashCode(hash, imageHandle->imageLayout, imageHandle->imageView, imageHandle->sampler);
#endif
			}
		}

		HashCode::hashCode(hash, desc.clearTargets);
		HashCode::hashCode(hash, desc.depthTarget);
		HashCode::hashCode(hash, desc.depthArrayTarget);
		HashCode::hashCode(hash, desc.swapChainTarget);

		if (desc.swapChainTarget)
		{
			auto texture = Application::getGraphicsContext()->getSwapChain()->getCurrentImage();
			if (texture)
			{
				HashCode::hashCode(hash, texture);
				HashCode::hashCode(hash, texture->getWidth(), texture->getHeight());
				HashCode::hashCode(hash, texture->getHandle());
				HashCode::hashCode(hash, texture->getFormat());
#ifdef MAPLE_VULKAN
				VkDescriptorImageInfo *imageHandle = (VkDescriptorImageInfo *) (texture->getDescriptorInfo());
				HashCode::hashCode(hash, imageHandle->imageLayout, imageHandle->imageView, imageHandle->sampler);
#endif
			}
		}
		auto &pipelineCache = Application::getGraphicsContext()->getPipelineCache();
		auto  found         = pipelineCache.find(hash);
		if (found != pipelineCache.end() && found->second)
		{
			return found->second;
		}

#ifdef MAPLE_OPENGL
		return pipelineCache.emplace(hash, std::make_shared<GLPipeline>(desc)).first->second;
#endif        // MAPLE_OPENGL

#ifdef MAPLE_VULKAN
		return pipelineCache.emplace(hash, std::make_shared<VulkanPipeline>(desc)).first->second;
#endif        // MAPLE_OPENGL
	}

	auto RenderPass::create(const RenderPassInfo &desc) -> std::shared_ptr<RenderPass>
	{
		return std::make_shared<GLRenderPass>(desc);
	}

	auto UniformBuffer::create() -> std::shared_ptr<UniformBuffer>
	{
		return std::make_shared<GLUniformBuffer>();
	}

	auto UniformBuffer::create(uint32_t size, const void *data) -> std::shared_ptr<UniformBuffer>
	{
		auto buffer = std::make_shared<GLUniformBuffer>();
		buffer->setData(size, data);
		return buffer;
	}

	auto VertexBuffer::create(const BufferUsage &usage) -> std::shared_ptr<VertexBuffer>
	{
		return std::make_shared<GLVertexBuffer>(usage);
	}

}        // namespace maple
