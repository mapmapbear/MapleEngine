//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "RHI/StorageBuffer.h"
#include "Vk.h"

namespace maple
{
	class VulkanBuffer;

	class VulkanStorageBuffer : public StorageBuffer
	{
	  public:
		VulkanStorageBuffer(const BufferOptions &options);
		VulkanStorageBuffer(uint32_t size, const void *data, const BufferOptions &options);
		auto setData(uint32_t size, const void *data) -> void override;
		auto getHandle() -> VkBuffer &;
		auto mapMemory(const std::function<void(void *)> &call) -> void override;
		auto unmap() -> void override;
		auto map() -> void * override;
	  private:
		std::shared_ptr<VulkanBuffer> vulkanBuffer;
		BufferOptions                 options;
	};
};        // namespace maple