//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "Others/HashCode.h"

#ifdef MAPLE_VULKAN
#	include "RHI/ImGui/VKImGuiRenderer.h"
#	include "RHI/Vulkan/VulkanCommandBuffer.h"
#	include "RHI/Vulkan/VulkanContext.h"
#	include "RHI/Vulkan/VulkanDescriptorSet.h"
#	include "RHI/Vulkan/VulkanFrameBuffer.h"
#	include "RHI/Vulkan/VulkanIndexBuffer.h"
#	include "RHI/Vulkan/VulkanPipeline.h"
#	include "RHI/Vulkan/VulkanRenderPass.h"
#	include "RHI/Vulkan/VulkanShader.h"
#	include "RHI/Vulkan/VulkanSwapChain.h"
#	include "RHI/Vulkan/VulkanUniformBuffer.h"
#	include "RHI/Vulkan/VulkanVertexBuffer.h"
#endif        // MAPLE_VULKAN

#ifdef MAPLE_OPENGL
#	include "RHI/ImGui/GLImGuiRenderer.h"
#	include "RHI/OpenGL/GLCommandBuffer.h"
#	include "RHI/OpenGL/GLContext.h"
#	include "RHI/OpenGL/GLDescriptorSet.h"
#	include "RHI/OpenGL/GLFrameBuffer.h"
#	include "RHI/OpenGL/GLIndexBuffer.h"
#	include "RHI/OpenGL/GLPipeline.h"
#	include "RHI/OpenGL/GLRenderPass.h"
#	include "RHI/OpenGL/GLShader.h"
#	include "RHI/OpenGL/GLSwapChain.h"
#	include "RHI/OpenGL/GLUniformBuffer.h"
#	include "RHI/OpenGL/GLVertexBuffer.h"
#endif

#include "Engine/CaptureGraph.h"

#include "Application.h"

namespace maple
{
	auto GraphicsContext::create() -> std::shared_ptr<GraphicsContext>
	{
#ifdef MAPLE_VULKAN
		return std::make_shared<VulkanContext>();
#endif        // MAPLE_VULKAN
#ifdef MAPLE_OPENGL
		return std::make_shared<GLContext>();
#endif        // MAPLE_OPENGL
	}

	auto GraphicsContext::clearUnused() -> void
	{
		auto current = Application::getTimer().currentTimestamp();

		for (auto iter = pipelineCache.begin(); iter != pipelineCache.end();)
		{
			if (iter->second.asset.use_count() == 1 && (current - iter->second.lastTimestamp) > 12000)
			{
				LOGI("Pipeline clear");
				iter = pipelineCache.erase(iter);
				continue;
			}
			iter++;
		}

		for (auto iter = frameBufferCache.begin(); iter != frameBufferCache.end();)
		{
			if (iter->second.asset.use_count() == 1 && (current - iter->second.lastTimestamp) > 12000)
			{
				LOGI("FrameBuffer clear");
				iter = frameBufferCache.erase(iter);
				continue;
			}
			iter++;
		}
	}

	auto SwapChain::create(uint32_t width, uint32_t height) -> std::shared_ptr<SwapChain>
	{
#ifdef MAPLE_VULKAN
		return std::make_shared<VulkanSwapChain>(width, height);
#endif
#ifdef MAPLE_OPENGL
		return std::make_shared<GLSwapChain>(width, height);
#endif
	}

	auto Shader::create(const std::string &filePath) -> std::shared_ptr<Shader>
	{
#ifdef MAPLE_VULKAN
		return Application::getCache()->emplace<VulkanShader>(filePath);
#endif
#ifdef MAPLE_OPENGL
		return Application::getCache()->emplace<GLShader>(filePath);
#endif
	}

	auto Shader::create(const std::vector<uint32_t> &vertData, const std::vector<uint32_t> &fragData) -> std::shared_ptr<Shader>
	{
#ifdef MAPLE_VULKAN
		return Application::getCache()->emplace<VulkanShader>(vertData, fragData);
#endif
#ifdef MAPLE_OPENGL
		return Application::getCache()->emplace<GLShader>(vertData, fragData);
#endif
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
		if (found != frameBufferCache.end() && found->second.asset)
		{
			found->second.lastTimestamp = Application::getTimer().currentTimestamp();
			return found->second.asset;
		}
#ifdef MAPLE_VULKAN

		std::shared_ptr<FrameBuffer> fb = std::make_shared<VulkanFrameBuffer>(desc);
		return frameBufferCache.emplace(std::piecewise_construct, std::forward_as_tuple(hash), std::forward_as_tuple(fb, Application::getTimer().currentTimestamp())).first->second.asset;
#endif
#ifdef MAPLE_OPENGL
		std::shared_ptr<FrameBuffer> fb = std::make_shared<GLFrameBuffer>(desc);
		return frameBufferCache.emplace(std::piecewise_construct, std::forward_as_tuple(hash), std::forward_as_tuple(fb, Application::getTimer().currentTimestamp())).first->second.asset;
#endif
	}

	auto DescriptorSet::create(const DescriptorInfo &desc) -> std::shared_ptr<DescriptorSet>
	{
#ifdef MAPLE_VULKAN
		return std::make_shared<VulkanDescriptorSet>(desc);
#endif
#ifdef MAPLE_OPENGL
		return std::make_shared<GLDescriptorSet>(desc);
#endif
	}

	auto CommandBuffer::create() -> std::shared_ptr<CommandBuffer>
	{
#ifdef MAPLE_VULKAN
		return std::make_shared<VulkanCommandBuffer>();
#endif
#ifdef MAPLE_OPENGL
		return std::make_shared<GLCommandBuffer>();
#endif
	}

	auto ImGuiRenderer::create(uint32_t width, uint32_t height, bool clearScreen) -> std::shared_ptr<ImGuiRenderer>
	{
#ifdef MAPLE_VULKAN
		return std::make_shared<VKImGuiRenderer>(width, height, clearScreen);
#endif
#ifdef MAPLE_OPENGL
		return std::make_shared<GLImGuiRenderer>(width, height, clearScreen);
#endif
	}

	auto IndexBuffer::create(const uint16_t *data, uint32_t count, BufferUsage bufferUsage) -> std::shared_ptr<IndexBuffer>
	{
#ifdef MAPLE_VULKAN
		return std::make_shared<VulkanIndexBuffer>(data, count, bufferUsage);
#endif
#ifdef MAPLE_OPENGL
		return std::make_shared<GLIndexBuffer>(data, count, bufferUsage);
#endif
	}
	auto IndexBuffer::create(const uint32_t *data, uint32_t count, BufferUsage bufferUsage) -> std::shared_ptr<IndexBuffer>
	{
#ifdef MAPLE_VULKAN
		return std::make_shared<VulkanIndexBuffer>(data, count, bufferUsage);
#endif
#ifdef MAPLE_OPENGL
		return std::make_shared<GLIndexBuffer>(data, count, bufferUsage);
#endif
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
		HashCode::hashCode(hash, desc.groupCountX, desc.groupCountY, desc.groupCountZ);

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

		if (found != pipelineCache.end() && found->second.asset)
		{
			found->second.lastTimestamp = Application::getTimer().currentTimestamp();
			return found->second.asset;
		}

#ifdef MAPLE_OPENGL
		std::shared_ptr<Pipeline> pipeline = std::make_shared<GLPipeline>(desc);
		return pipelineCache.emplace(std::piecewise_construct, std::forward_as_tuple(hash), std::forward_as_tuple(pipeline, Application::getTimer().currentTimestamp())).first->second.asset;
#endif        // MAPLE_OPENGL

#ifdef MAPLE_VULKAN
		std::shared_ptr<Pipeline> pipeline = std::make_shared<VulkanPipeline>(desc);
		return pipelineCache.emplace(std::piecewise_construct, std::forward_as_tuple(hash), std::forward_as_tuple(pipeline, Application::getTimer().currentTimestamp())).first->second.asset;
#endif        // MAPLE_OPENGL
	}


	auto Pipeline::get(const PipelineInfo& desc, const std::vector<std::shared_ptr<DescriptorSet>>& sets, capture_graph::component::RenderGraph & graph) -> std::shared_ptr<Pipeline>
	{
		auto pip = Pipeline::get(desc);
		
		for (auto set : sets)
		{
			for (auto input : set->getDescriptors())
			{
				capture_graph::input(desc.shader->getName(), graph, input.textures);
			}
		}

		capture_graph::input(desc.shader->getName(), graph, { desc.depthTarget,desc.depthArrayTarget });

		for (auto color : desc.colorTargets)
		{	
			if(color!=nullptr)
				capture_graph::output(desc.shader->getName(), graph, { color });
		}
	
		return pip;
	}

	auto RenderPass::create(const RenderPassInfo &desc) -> std::shared_ptr<RenderPass>
	{
#ifdef MAPLE_VULKAN
		return std::make_shared<VulkanRenderPass>(desc);
#endif
#ifdef MAPLE_OPENGL
		return std::make_shared<GLRenderPass>(desc);
#endif
	}

	auto UniformBuffer::create() -> std::shared_ptr<UniformBuffer>
	{
#ifdef MAPLE_VULKAN
		return std::make_shared<VulkanUniformBuffer>();
#endif
#ifdef MAPLE_OPENGL
		return std::make_shared<GLUniformBuffer>();
#endif
	}

	auto UniformBuffer::create(uint32_t size, const void *data) -> std::shared_ptr<UniformBuffer>
	{
#ifdef MAPLE_VULKAN
		auto buffer = std::make_shared<VulkanUniformBuffer>();
#endif
#ifdef MAPLE_OPENGL
		auto buffer = std::make_shared<GLUniformBuffer>();
#endif
		buffer->setData(size, data);
		return buffer;
	}

	auto VertexBuffer::create(const BufferUsage &usage) -> std::shared_ptr<VertexBuffer>
	{
#ifdef MAPLE_VULKAN
		return std::make_shared<VulkanVertexBuffer>(usage);
#endif
#ifdef MAPLE_OPENGL
		return std::make_shared<GLVertexBuffer>(usage);
#endif
	}
}        // namespace maple
