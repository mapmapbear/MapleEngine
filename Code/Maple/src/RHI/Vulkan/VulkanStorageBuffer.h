//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Vk.h"
#include "RHI/StorageBuffer.h"

namespace maple
{
	class VulkanBuffer;

	class VulkanStorageBuffer : public StorageBuffer 
	{
	public:
		VulkanStorageBuffer();
		VulkanStorageBuffer(uint32_t size, const void* data);
		auto setData(uint32_t size, const void* data) -> void override;
		auto getHandle() -> VkBuffer&;
	private:
		std::shared_ptr<VulkanBuffer> vulkanBuffer;
	};
};