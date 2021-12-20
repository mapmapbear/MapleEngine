//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "VulkanFrameBuffer.h"
#include "Others/Console.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanHelper.h"
#include "VulkanRenderPass.h"
#include "VulkanTexture.h"

namespace maple
{
	VulkanFrameBuffer::VulkanFrameBuffer(const FrameBufferInfo &info) :
	    info(info)
	{
		std::vector<VkImageView> attachments;

		//apply attachment for the framebuffer
		for (uint32_t i = 0; i < info.attachments.size(); i++)
		{
			switch (info.attachments[i]->getType())
			{
					//color attachment
				case TextureType::Color:
					attachments.emplace_back(static_cast<VulkanTexture2D *>(info.attachments[i].get())->getImageView());
					break;
					//depth attachment for depth test
				case TextureType::Depth:
					attachments.emplace_back(static_cast<VulkanTextureDepth *>(info.attachments[i].get())->getImageView());
					break;

				case TextureType::DepthArray:
					attachments.emplace_back(static_cast<VulkanTextureDepthArray *>(info.attachments[i].get())->getImageView(info.layer));
					break;

				case TextureType::Cube:
					attachments.emplace_back(static_cast<VulkanTextureCube *>(info.attachments[i].get())->getImageView());
					break;
			}
		}

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass      = *std::static_pointer_cast<VulkanRenderPass>(info.renderPass);
		framebufferInfo.attachmentCount = attachments.size();
		framebufferInfo.pAttachments    = attachments.data();
		framebufferInfo.width           = info.width;
		framebufferInfo.height          = info.height;
		framebufferInfo.layers          = 1;
		framebufferInfo.pNext           = nullptr;
		framebufferInfo.flags           = 0;

		VK_CHECK_RESULT(vkCreateFramebuffer(*VulkanDevice::get(), &framebufferInfo, nullptr, &buffer));
	}

	VulkanFrameBuffer::~VulkanFrameBuffer()
	{
		auto &deletionQueue = VulkanContext::get()->getDeletionQueue();
		auto  framebuffer   = this->buffer;
		deletionQueue.emplace([framebuffer] { vkDestroyFramebuffer(*VulkanDevice::get(), framebuffer, VK_NULL_HANDLE); });
	}

};        // namespace maple
