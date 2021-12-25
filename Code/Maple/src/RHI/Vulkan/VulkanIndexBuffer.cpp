//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "VulkanIndexBuffer.h"
#include "Engine/Profiler.h"
#include "VulkanCommandBuffer.h"

namespace maple
{
	VulkanIndexBuffer::VulkanIndexBuffer(const uint16_t *data, uint32_t initCount, BufferUsage bufferUsage) :
	    VulkanBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, initCount * sizeof(uint16_t), data), size(initCount * sizeof(uint16_t)), count(initCount), usage(bufferUsage)
	{
	}

	VulkanIndexBuffer::VulkanIndexBuffer(const uint32_t *data, uint32_t initCount, BufferUsage bufferUsage) :
	    VulkanBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, initCount * sizeof(uint32_t), data), size(initCount * sizeof(uint32_t)), count(initCount), usage(bufferUsage)
	{
	}

	VulkanIndexBuffer::~VulkanIndexBuffer()
	{
		if (mappedBuffer)
		{
			VulkanBuffer::flush(size);
			VulkanBuffer::unmap();
			mappedBuffer = false;
		}
	}

	auto VulkanIndexBuffer::bind(CommandBuffer *commandBuffer) const -> void
	{
		vkCmdBindIndexBuffer(static_cast<VulkanCommandBuffer *>(commandBuffer)->getCommandBuffer(), buffer, 0, VK_INDEX_TYPE_UINT32);
	}

	auto VulkanIndexBuffer::unbind() const -> void
	{
	}

	auto VulkanIndexBuffer::setData(uint32_t size, const void *data) -> void
	{
		setVkData(size, data);
	}

	auto VulkanIndexBuffer::releasePointer() -> void
	{
		PROFILE_FUNCTION();
		if (mappedBuffer)
		{
			VulkanBuffer::flush(size);
			VulkanBuffer::unmap();
			mappedBuffer = false;
		}
	}

	auto VulkanIndexBuffer::getPointerInternal() -> void * 
	{
		if (!mappedBuffer)
		{
			VulkanBuffer::map();
			mappedBuffer = true;
		}
		return mapped;
	}
};        // namespace maple
