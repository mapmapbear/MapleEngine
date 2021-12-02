//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "VulkanHelper.h"
namespace maple
{
	class VulkanCommandPool
	{
	public:
		VulkanCommandPool();
		~VulkanCommandPool();

		auto init() -> void;
		autoUnpack(commandPool);
	private:
		VkCommandPool commandPool;
	};
};