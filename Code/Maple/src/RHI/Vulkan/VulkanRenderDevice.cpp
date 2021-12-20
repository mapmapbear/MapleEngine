//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "VulkanRenderDevice.h"
#include "VulkanCommandBuffer.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanTexture.h"
#include "VulkanPipeline.h"

#include "Engine/Core.h"
#include "Engine/Profiler.h"
#include "Others/Console.h"

#include "VulkanDescriptorSet.h"
#include "VulkanFramebuffer.h"

#include "RHI/Texture.h"

#include "Application.h"

namespace maple
{
	static constexpr uint32_t MAX_DESCRIPTOR_SET_COUNT = 1500;


	VulkanRenderDevice::VulkanRenderDevice()
	{
	}

	VulkanRenderDevice::~VulkanRenderDevice()
	{
	}

	auto VulkanRenderDevice::init() -> void
	{
		PROFILE_FUNCTION();
		VkDescriptorPoolSize poolSizes[] = {
		    {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
		    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
		    {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
		    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
		    {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
		    {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
		    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
		    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
		    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
		    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
		    {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

		// Create info
		VkDescriptorPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCreateInfo.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolCreateInfo.poolSizeCount              = 11;
		poolCreateInfo.pPoolSizes                 = poolSizes;
		poolCreateInfo.maxSets                    = MAX_DESCRIPTOR_SET_COUNT;

		// Pool
		VK_CHECK_RESULT(vkCreateDescriptorPool(*VulkanDevice::get(), &poolCreateInfo, nullptr, &descriptorPool));
	}

	auto VulkanRenderDevice::begin() -> void
	{
		PROFILE_FUNCTION();
		auto swapChain = Application::getGraphicsContext()->getSwapChain();
		std::static_pointer_cast<VulkanSwapChain>(swapChain)->begin();
	}

	auto VulkanRenderDevice::presentInternal() -> void
	{
		PROFILE_FUNCTION();
		auto swapChain = std::static_pointer_cast<VulkanSwapChain>(Application::getGraphicsContext()->getSwapChain());

		swapChain->end();
		swapChain->queueSubmit();

		auto &frameData = swapChain->getFrameData();
		auto  semphore  = frameData.commandBuffer->getSemaphore();
		swapChain->present(semphore);
		swapChain->acquireNextImage();
	}

	auto VulkanRenderDevice::presentInternal(CommandBuffer *commandBuffer) -> void
	{
		PROFILE_FUNCTION();
	}

	auto VulkanRenderDevice::onResize(uint32_t width, uint32_t height) -> void
	{
		PROFILE_FUNCTION();
		if (width == 0 || height == 0)
			return;

		VulkanHelper::validateResolution(width, height);

		std::static_pointer_cast<VulkanSwapChain>(Application::getGraphicsContext()->getSwapChain())->onResize(width, height);
	}

	auto VulkanRenderDevice::drawInternal(CommandBuffer *commandBuffer, const DrawType type, uint32_t count, DataType dataType, const void *indices) const -> void
	{
		PROFILE_FUNCTION();
		//NumDrawCalls++;
		vkCmdDraw(*static_cast<VulkanCommandBuffer *>(commandBuffer), count, 1, 0, 0);
	}

	auto VulkanRenderDevice::drawIndexedInternal(CommandBuffer *commandBuffer, const DrawType type, uint32_t count, uint32_t start) const -> void
	{
		PROFILE_FUNCTION();
		vkCmdDrawIndexed(*static_cast<VulkanCommandBuffer *>(commandBuffer), count, 1, 0, 0, 0);
	}

	auto VulkanRenderDevice::bindDescriptorSetsInternal(Pipeline *pipeline, CommandBuffer *commandBuffer, uint32_t dynamicOffset, const std::vector<std::shared_ptr<DescriptorSet>> &descriptorSets) -> void
	{
		PROFILE_FUNCTION();
		uint32_t numDynamicDescriptorSets = 0;
		uint32_t numDesciptorSets         = 0;

		for (auto &descriptorSet : descriptorSets)
		{
			if (descriptorSet)
			{
				auto vkDesSet = std::static_pointer_cast<VulkanDescriptorSet>(descriptorSet);
				if (vkDesSet->isDynamic())
					numDynamicDescriptorSets++;

				descriptorSetPool[numDesciptorSets] = vkDesSet->getDescriptorSet();
				numDesciptorSets++;
			}
		}

		vkCmdBindDescriptorSets(*static_cast<VulkanCommandBuffer *>(commandBuffer), VK_PIPELINE_BIND_POINT_GRAPHICS, static_cast<VulkanPipeline *>(pipeline)->getPipelineLayout(), 0, numDesciptorSets, descriptorSetPool, numDynamicDescriptorSets, &dynamicOffset);
	}

	auto VulkanRenderDevice::clearRenderTarget(const std::shared_ptr<Texture> &texture, CommandBuffer *commandBuffer, const glm::vec4 &clearColor) -> void
	{
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.baseMipLevel            = 0;
		subresourceRange.layerCount              = 1;
		subresourceRange.levelCount              = 1;
		subresourceRange.baseArrayLayer          = 0;

		if (texture->getType() == TextureType::Color)
		{
			auto vkTexture = (VulkanTexture2D *) texture.get();

			VkImageLayout layout = vkTexture->getImageLayout();
			((VulkanTexture2D *) texture.get())->transitionImage(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (VulkanCommandBuffer *) commandBuffer);
			subresourceRange.aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT;
			VkClearColorValue clearColourValue = VkClearColorValue({{clearColor.x, clearColor.y, clearColor.z, clearColor.w}});
			vkCmdClearColorImage(*((VulkanCommandBuffer *) commandBuffer), vkTexture->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColourValue, 1, &subresourceRange);
			vkTexture->transitionImage(layout, (VulkanCommandBuffer *) commandBuffer);
		}
		else if (texture->getType() == TextureType::Depth)
		{
			auto vkTexture = (VulkanTextureDepth *) texture.get();

			VkClearDepthStencilValue clearDepthStencil = {1.0f, 1};
			subresourceRange.aspectMask                = VK_IMAGE_ASPECT_DEPTH_BIT;
			vkTexture->transitionImage(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (VulkanCommandBuffer *) commandBuffer);
			vkCmdClearDepthStencilImage(*((VulkanCommandBuffer *) commandBuffer), vkTexture->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearDepthStencil, 1, &subresourceRange);
		}
	}

}        // namespace maple
