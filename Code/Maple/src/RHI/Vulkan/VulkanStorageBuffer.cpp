//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "VulkanStorageBuffer.h"
#include "VulkanBuffer.h"

namespace maple
{

	VulkanStorageBuffer::VulkanStorageBuffer(uint32_t size, const void* data)
	{
		vulkanBuffer = std::make_shared<VulkanBuffer>(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, size, data);
	}

	VulkanStorageBuffer::VulkanStorageBuffer()
	{
		vulkanBuffer = std::make_shared<VulkanBuffer>();
	}

	auto VulkanStorageBuffer::setData(uint32_t size, const void* data) -> void 
	{
		if (vulkanBuffer->getSize() == 0) 
		{
			vulkanBuffer->init(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, size, data);
		}
		vulkanBuffer->setVkData(size, data);
	}

	auto VulkanStorageBuffer::getHandle() -> VkBuffer&
	{
		return vulkanBuffer->getVkBuffer();
	}
}