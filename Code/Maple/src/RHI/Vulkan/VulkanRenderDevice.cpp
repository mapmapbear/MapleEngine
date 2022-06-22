//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "VulkanRenderDevice.h"
#include "VulkanCommandBuffer.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanPipeline.h"
#include "VulkanSwapChain.h"
#include "VulkanTexture.h"

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
		PROFILE_FUNCTION();
		vkDestroyDescriptorPool(*VulkanDevice::get(), descriptorPool, VK_NULL_HANDLE);
	}

	auto VulkanRenderDevice::init() -> void
	{
		PROFILE_FUNCTION();

		std::vector<VkDescriptorPoolSize> poolSizes = {
		    VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_SAMPLER, 500},
		    VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 500},
		    VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 500},
		    VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 500},
		    VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 500},
		    VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 500}};

		if (VulkanDevice::get()->getPhysicalDevice()->isRaytracingSupport())
		{
			poolSizes.push_back({VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 16});
		}

		// Create info
		VkDescriptorPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCreateInfo.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolCreateInfo.poolSizeCount              = poolSizes.size();
		poolCreateInfo.pPoolSizes                 = poolSizes.data();
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

		swapChain->present();
		swapChain->acquireNextImage();
	}

	auto VulkanRenderDevice::presentInternal(const CommandBuffer *commandBuffer) -> void
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

	auto VulkanRenderDevice::drawInternal(const CommandBuffer *commandBuffer, const DrawType type, uint32_t count, DataType dataType, const void *indices) const -> void
	{
		PROFILE_FUNCTION();
		//NumDrawCalls++;
		vkCmdDraw(static_cast<const VulkanCommandBuffer *>(commandBuffer)->getCommandBuffer(), count, 1, 0, 0);
	}

	auto VulkanRenderDevice::drawArraysInternal(const CommandBuffer *commandBuffer, DrawType type, uint32_t count, uint32_t start /*= 0*/) const -> void
	{
		PROFILE_FUNCTION();
		vkCmdDraw(static_cast<const VulkanCommandBuffer *>(commandBuffer)->getCommandBuffer(), count, 1, 0, 0);
	}

	auto VulkanRenderDevice::drawIndexedInternal(const CommandBuffer *commandBuffer, const DrawType type, uint32_t count, uint32_t start) const -> void
	{
		PROFILE_FUNCTION();
		vkCmdDrawIndexed(static_cast<const VulkanCommandBuffer *>(commandBuffer)->getCommandBuffer(), count, 1, start, 0, 0);
	}

	auto VulkanRenderDevice::bindDescriptorSetsInternal(Pipeline *pipeline, const CommandBuffer *commandBuffer, uint32_t dynamicOffset, const std::vector<std::shared_ptr<DescriptorSet>> &descriptorSets) -> void
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

		vkCmdBindDescriptorSets(
		    static_cast<const VulkanCommandBuffer *>(commandBuffer)->getCommandBuffer(),
		    static_cast<const VulkanPipeline *>(pipeline)->getPipelineBindPoint(),
		    static_cast<const VulkanPipeline *>(pipeline)->getPipelineLayout(), 0, numDesciptorSets, descriptorSetPool, numDynamicDescriptorSets, &dynamicOffset);
	}

	auto VulkanRenderDevice::clearRenderTarget(const std::shared_ptr<Texture> &texture, const CommandBuffer *commandBuffer, const glm::vec4 &clearColor) -> void
	{
		PROFILE_FUNCTION();
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.baseMipLevel            = 0;
		subresourceRange.layerCount              = 1;
		subresourceRange.levelCount              = 1;
		subresourceRange.baseArrayLayer          = 0;

		if (texture->getType() == TextureType::Color)
		{
			auto vkTexture = (VulkanTexture2D *) texture.get();

			VkImageLayout layout = vkTexture->getImageLayout();
			vkTexture->transitionImage(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (const VulkanCommandBuffer *) commandBuffer);
			subresourceRange.aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT;
			VkClearColorValue clearColourValue = VkClearColorValue({{clearColor.x, clearColor.y, clearColor.z, clearColor.w}});
			vkCmdClearColorImage(((const VulkanCommandBuffer *) commandBuffer)->getCommandBuffer(), vkTexture->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColourValue, 1, &subresourceRange);
			vkTexture->transitionImage(layout, (const VulkanCommandBuffer *) commandBuffer);
		}
		else if (texture->getType() == TextureType::Depth)
		{
			auto vkTexture = (VulkanTextureDepth *) texture.get();

			VkClearDepthStencilValue clearDepthStencil = {1.0f, 0};
			subresourceRange.aspectMask                = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			vkTexture->transitionImage(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (const VulkanCommandBuffer *) commandBuffer);
			vkCmdClearDepthStencilImage(((const VulkanCommandBuffer *) commandBuffer)->getCommandBuffer(), vkTexture->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearDepthStencil, 1, &subresourceRange);
		}
	}

	auto VulkanRenderDevice::dispatch(const CommandBuffer *commandBuffer, uint32_t x, uint32_t y, uint32_t z) -> void
	{
		PROFILE_FUNCTION();
		vkCmdDispatch(((const VulkanCommandBuffer *) commandBuffer)->getCommandBuffer(), x, y, z);
	}

	auto VulkanRenderDevice::memoryBarrier(const CommandBuffer *commandBuffer, uint32_t flag) -> void
	{
		PROFILE_FUNCTION();
	}

}        // namespace maple
