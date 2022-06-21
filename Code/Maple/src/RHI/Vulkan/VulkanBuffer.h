//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Vk.h"

#include "Engine/Core.h"
namespace maple
{
	class VulkanBuffer
	{
	  public:
		using Ptr = std::shared_ptr<VulkanBuffer>;

		VulkanBuffer();
		VulkanBuffer(VkBufferUsageFlags usage, uint32_t size, const void *data, bool gpuOnly = false);
		virtual ~VulkanBuffer();
		NO_COPYABLE(VulkanBuffer);

		auto resize(uint32_t size, const void *data) -> void;
		auto init(VkBufferUsageFlags usage, uint32_t size, const void *data, bool gpuOnly = false) -> void;
		auto map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) -> void;
		auto unmap() -> void;
		auto flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) -> void;
		auto invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) -> void;
		auto setVkData(uint32_t size, const void *data, uint32_t offset = 0) -> void;

		inline auto setUsage(VkBufferUsageFlags flags)
		{
			usage = flags;
		}
		inline auto &getVkBuffer()
		{
			return buffer;
		}
		inline const auto &getVkBuffer() const
		{
			return buffer;
		}
		inline const auto &getBufferInfo() const
		{
			return desciptorBufferInfo;
		}

		auto getDeviceAddress() const -> VkDeviceAddress;

		inline auto getSize() const
		{
			return size;
		}

	  protected:
		auto release() -> void;

		VkDescriptorBufferInfo desciptorBufferInfo{};
		VkBuffer               buffer    = VK_NULL_HANDLE;
		VkDeviceMemory         memory    = VK_NULL_HANDLE;
		VkDeviceSize           size      = 0;
		VkDeviceSize           alignment = 0;
		void *                 mapped    = nullptr;
		VkBufferUsageFlags     usage;
#ifdef USE_VMA_ALLOCATOR
		VmaAllocation allocation{};
		VmaAllocation mappedAllocation{};
#endif
	};
}        // namespace maple
