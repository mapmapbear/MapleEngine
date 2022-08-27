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
		VulkanStorageBuffer(uint32_t size, uint32_t flags, const BufferOptions &options);
		VulkanStorageBuffer(uint32_t size, const void *data, const BufferOptions &options);
		auto setData(uint32_t size, const void *data) -> void override;
		auto getHandle() const -> VkBuffer &;
		auto mapMemory(const std::function<void(void *)> &call) -> void override;
		auto unmap() -> void override;
		auto map() -> void * override;
		auto getDeviceAddress() const -> uint64_t override;

		inline auto getAccessFlagBits() const
		{
			return accessFlagBits;
		}

		auto setAccessFlagBits(uint32_t flags) -> void;

		inline auto isIndirect() const
		{
			return options.indirect;
		}

	private:
		std::shared_ptr<VulkanBuffer> vulkanBuffer;
		BufferOptions                 options;
		uint32_t                      accessFlagBits;
		uint32_t                      lastAccessFlagBits;
	};
};        // namespace maple