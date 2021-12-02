//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "vulkan/vulkan.h"
namespace maple
{
	class VulkanBuffer
	{
	public:
		VulkanBuffer();
		VulkanBuffer(VkBufferUsageFlags usage, uint32_t size, const void* data);
		virtual ~VulkanBuffer();
		auto init(VkBufferUsageFlags usage, uint32_t size, const void* data) -> void;
		auto map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)->VkResult;
		auto unmap() -> void;
		auto flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) -> void;
		auto invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) -> void;
		auto setVkData(uint32_t size, const void* data,uint32_t offset = 0)->void;

		inline auto setUsage(VkBufferUsageFlags flags) { usage = flags; }
		inline auto& getBuffer() { return buffer; }
		inline const auto& getBuffer() const { return buffer; }
		inline const auto& getBufferInfo() const { return desciptorBufferInfo; }
		virtual auto resize(uint32_t size, const void* data) -> void;
	protected:

		auto release() -> void;

		VkDescriptorBufferInfo desciptorBufferInfo{};
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkDeviceSize size = 0;
		VkDeviceSize alignment = 0;
		void* mapped = nullptr;

		VkBufferUsageFlags usage;
	};
}