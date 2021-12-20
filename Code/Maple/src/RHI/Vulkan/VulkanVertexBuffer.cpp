//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "VulkanVertexBuffer.h"
#include "Engine/Profiler.h"
#include "VulkanCommandBuffer.h"
#include "VulkanPipeline.h"

namespace maple
{
	VulkanVertexBuffer::VulkanVertexBuffer(const BufferUsage &usage) :
	    bufferUsage(usage)
	{
		VulkanBuffer::setUsage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	}

	VulkanVertexBuffer::~VulkanVertexBuffer()
	{
		releasePointer();
	}

	auto VulkanVertexBuffer::resize(uint32_t size) -> void
	{
		PROFILE_FUNCTION();
		if (this->size != size)
		{
			this->size = size;
			VulkanBuffer::resize(size, nullptr);
		}
	}

	auto VulkanVertexBuffer::setData(uint32_t size, const void *data) -> void
	{
		PROFILE_FUNCTION();
		if (size != this->size)
		{
			this->size = size;
			VulkanBuffer::resize(size, data);
		}
		else
		{
			VulkanBuffer::setVkData(size, data);
		}
	}

	auto VulkanVertexBuffer::setDataSub(uint32_t size, const void *data, uint32_t offset) -> void
	{
		PROFILE_FUNCTION();
		if (size != this->size)
		{
			this->size = size;
			VulkanBuffer::resize(size, data);
		}
		else
		{
			VulkanBuffer::setVkData(size, data);
		}
	}

	auto VulkanVertexBuffer::releasePointer() -> void
	{
		PROFILE_FUNCTION();
		if (mappedBuffer)
		{
			VulkanBuffer::flush(size);
			VulkanBuffer::unmap();
			mappedBuffer = false;
		}
	}

	auto VulkanVertexBuffer::bind(CommandBuffer *commandBuffer, Pipeline *pipeline) -> void
	{
		PROFILE_FUNCTION();
		VkDeviceSize offsets[1] = {0};
		if (commandBuffer)
			vkCmdBindVertexBuffers(*static_cast<VulkanCommandBuffer *>(commandBuffer), 0, 1, &buffer, offsets);
	}

	auto VulkanVertexBuffer::unbind() -> void
	{
	}

	auto VulkanVertexBuffer::getPointerInternal() -> void *
	{
		PROFILE_FUNCTION();
		if (!mappedBuffer)
		{
			VulkanBuffer::map();
			mappedBuffer = true;
		}
		return mapped;
	}

};        // namespace maple
