//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "VulkanStorageBuffer.h"
#include "VulkanBuffer.h"

namespace maple
{
	VulkanStorageBuffer::VulkanStorageBuffer(uint32_t size, const void *data, const BufferOptions &options) :
	    options(options)
	{
		VkBufferUsageFlags flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

		if (options.indirect)
		{
			flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
		}

		vulkanBuffer = std::make_shared<VulkanBuffer>(flags, size, data, options.gpuOnly);
	}

	VulkanStorageBuffer::VulkanStorageBuffer(const BufferOptions &options) :
	    options(options)
	{
		vulkanBuffer = std::make_shared<VulkanBuffer>();
	}

	auto VulkanStorageBuffer::setData(uint32_t size, const void *data) -> void
	{
		if (vulkanBuffer->getSize() == 0)
		{
			VkBufferUsageFlags flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

			if (options.indirect)
			{
				flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
			}

			vulkanBuffer->init(flags, size, data);
		}
		else
		{
			vulkanBuffer->setVkData(size, data);
		}
	}

	auto VulkanStorageBuffer::getHandle() -> VkBuffer &
	{
		return vulkanBuffer->getVkBuffer();
	}

	auto VulkanStorageBuffer::mapMemory(const std::function<void(void *)> &call) -> void
	{
		vulkanBuffer->map();
		call(vulkanBuffer->getMapped());
		vulkanBuffer->unmap();
	}

	auto VulkanStorageBuffer::unmap() -> void
	{
		vulkanBuffer->unmap();
	}

	auto VulkanStorageBuffer::map() -> void *
	{
		vulkanBuffer->map();
		return vulkanBuffer->getMapped();
	}
}        // namespace maple