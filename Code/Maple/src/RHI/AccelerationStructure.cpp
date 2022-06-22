//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "AccelerationStructure.h"
#include "StorageBuffer.h"

#ifdef MAPLE_VULKAN
#	include "RHI/Vulkan/Vk.h"
#	include "RHI/Vulkan/VulkanContext.h"
#	include "RHI/Vulkan/Raytracing/RayTracingProperties.h"
#	include "RHI/Vulkan/Raytracing/VulkanAccelerationStructure.h"
#else
#endif        // MAPLE_VULKAN

namespace maple
{
	auto AccelerationStructure::createTopLevel(uint64_t address, uint32_t instancesCount) -> Ptr
	{
#ifdef MAPLE_VULKAN
		auto rayTracingProperties = std::make_shared<RayTracingProperties>();
		return std::make_shared<TopLevelAccelerationStructure>(rayTracingProperties, address, instancesCount);
#endif        // MAPLE_VULKAN
	}
}        // namespace maple