//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "AccelerationStructure.h"
#include "Others/Console.h"
#include "RHI/Vulkan/VulkanDevice.h"
#include "RayTracingProperties.h"

namespace maple
{
	namespace
	{
		inline auto roundUp(uint64_t size, uint64_t granularity)
		{
			const auto divUp = (size + granularity - 1) / granularity;
			return divUp * granularity;
		}
	}        // namespace

	AccelerationStructure::AccelerationStructure(const std::shared_ptr<RayTracingProperties> &rayTracingProperties) :
	    rayTracingProperties(rayTracingProperties)
	{
	}

	AccelerationStructure::~AccelerationStructure()
	{
		if (accelerationStructure != nullptr)
		{
			vkDestroyAccelerationStructureKHR(*VulkanDevice::get(), accelerationStructure, nullptr);
			accelerationStructure = nullptr;
		}
	}

	auto AccelerationStructure::getBuildSizes(const uint32_t *maxPrimitiveCounts) const -> const VkAccelerationStructureBuildSizesInfoKHR
	{
		// Query both the size of the finished acceleration structure and the amount of scratch memory needed.
		VkAccelerationStructureBuildSizesInfoKHR sizeInfo = {};
		sizeInfo.sType                                    = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

		vkGetAccelerationStructureBuildSizesKHR(
		    *VulkanDevice::get(),
		    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		    &buildGeometryInfo,
		    maxPrimitiveCounts,
		    &sizeInfo);

		// AccelerationStructure offset needs to be 256 bytes aligned (official Vulkan specs, don't ask me why).
		constexpr uint64_t accelerationStructureAlignment = 256;
		const uint64_t     scratchAlignment               = rayTracingProperties->getMinAccelerationStructureScratchOffsetAlignment();

		sizeInfo.accelerationStructureSize = roundUp(sizeInfo.accelerationStructureSize, accelerationStructureAlignment);
		sizeInfo.buildScratchSize          = roundUp(sizeInfo.buildScratchSize, scratchAlignment);

		return sizeInfo;
	}

	auto AccelerationStructure::memoryBarrier(VkCommandBuffer cmdBuffer) -> void
	{
		// Wait for the builder to complete by setting a barrier on the resulting buffer. This is
		// particularly important as the construction of the top-level hierarchy may be called right
		// afterwards, before executing the command list.

		VkMemoryBarrier memoryBarrier = {};
		memoryBarrier.sType           = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memoryBarrier.pNext           = nullptr;
		memoryBarrier.srcAccessMask   = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
		memoryBarrier.dstAccessMask   = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;

		vkCmdPipelineBarrier(
		    cmdBuffer,
		    VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
		    VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
		    0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
	}

	auto AccelerationStructure::createAccelerationStructure(const VulkanBuffer::Ptr &resultBuffer, VkDeviceSize resultOffset) -> void
	{
		VkAccelerationStructureCreateInfoKHR createInfo = {};
		createInfo.sType                                = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		createInfo.pNext                                = nullptr;
		createInfo.type                                 = buildGeometryInfo.type;
		createInfo.size                                 = getBuildSizes().accelerationStructureSize;
		createInfo.buffer                               = resultBuffer->getVkBuffer();
		createInfo.offset                               = resultOffset;
		VK_CHECK_RESULT(vkCreateAccelerationStructureKHR(*VulkanDevice::get(), &createInfo, nullptr, &accelerationStructure));
	}

	BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(const std::shared_ptr<RayTracingProperties> &rayTracingProperties, const BottomLevelGeometry &geometries) :
	    AccelerationStructure(rayTracingProperties),
	    geometries(geometries)
	{
	}

	BottomLevelAccelerationStructure::~BottomLevelAccelerationStructure()
	{
	}

	auto BottomLevelAccelerationStructure::generate(VkCommandBuffer commandBuffer, const VulkanBuffer::Ptr &scratchBuffer, VkDeviceSize scratchOffset, const VulkanBuffer::Ptr &resultBuffer, VkDeviceSize resultOffset) -> void
	{
		// Create the acceleration structure.
		createAccelerationStructure(resultBuffer, resultOffset);

		// Build the actual bottom-level acceleration structure
		const VkAccelerationStructureBuildRangeInfoKHR *pBuildOffsetInfo = geometries.getBuildOffsetInfo().data();

		buildGeometryInfo.dstAccelerationStructure  = accelerationStructure;
		buildGeometryInfo.scratchData.deviceAddress = scratchBuffer->getDeviceAddress() + scratchOffset;

		vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildGeometryInfo, &pBuildOffsetInfo);
	}

	TopLevelAccelerationStructure::TopLevelAccelerationStructure(const std::shared_ptr<RayTracingProperties> &rayTracingProperties, VkDeviceAddress instanceAddress, uint32_t instancesCount) :
	    AccelerationStructure(rayTracingProperties),
	    instancesCount(instancesCount)
	{
		// Create VkAccelerationStructureGeometryInstancesDataKHR. This wraps a device pointer to the above uploaded instances.
		instancesVk.sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		instancesVk.arrayOfPointers    = VK_FALSE;
		instancesVk.data.deviceAddress = instanceAddress;

		// Put the above into a VkAccelerationStructureGeometryKHR. We need to put the
		// instances struct in a union and label it as instance data.
		topASGeometry.sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		topASGeometry.geometryType       = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		topASGeometry.geometry.instances = instancesVk;

		buildGeometryInfo.sType                    = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		buildGeometryInfo.flags                    = flags;
		buildGeometryInfo.geometryCount            = 1;
		buildGeometryInfo.pGeometries              = &topASGeometry;
		buildGeometryInfo.mode                     = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		buildGeometryInfo.type                     = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		buildGeometryInfo.srcAccelerationStructure = nullptr;

		buildSizesInfo = getBuildSizes(&instancesCount);
	}

	TopLevelAccelerationStructure::~TopLevelAccelerationStructure()
	{}

	auto TopLevelAccelerationStructure::generate(VkCommandBuffer commandBuffer, const VulkanBuffer::Ptr &scratchBuffer, VkDeviceSize scratchOffset, const VulkanBuffer::Ptr &resultBuffer, VkDeviceSize resultOffset) -> void
	{
		createAccelerationStructure(resultBuffer, resultOffset);

		// Build the actual bottom-level acceleration structure
		VkAccelerationStructureBuildRangeInfoKHR buildOffsetInfo = {};
		buildOffsetInfo.primitiveCount                           = instancesCount;

		const VkAccelerationStructureBuildRangeInfoKHR *pBuildOffsetInfo = &buildOffsetInfo;

		buildGeometryInfo.dstAccelerationStructure  = accelerationStructure;
		buildGeometryInfo.scratchData.deviceAddress = scratchBuffer->getDeviceAddress() + scratchOffset;

		vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildGeometryInfo, &pBuildOffsetInfo);
	}

	auto TopLevelAccelerationStructure::createInstance(
	    const BottomLevelAccelerationStructure &bottomLevelAs,
	    const glm::mat4 &                       transform,
	    uint32_t                                instanceId,
	    uint32_t                                hitGroupId) -> VkAccelerationStructureInstanceKHR
	{
		VkAccelerationStructureDeviceAddressInfoKHR addressInfo = {};
		addressInfo.sType                                       = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		addressInfo.accelerationStructure                       = bottomLevelAs.getAccelerationStructure();

		const auto address = vkGetAccelerationStructureDeviceAddressKHR(*VulkanDevice::get(), &addressInfo);

		VkAccelerationStructureInstanceKHR instance     = {};
		instance.instanceCustomIndex                    = instanceId;
		instance.mask                                   = 0xFF;                                                             // The visibility mask is always set of 0xFF, but if some instances would need to be ignored in some cases, this flag should be passed by the application.
		instance.instanceShaderBindingTableRecordOffset = hitGroupId;                                                       // Set the hit group index, that will be used to find the shader code to execute when hitting the geometry.
		instance.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;        // Disable culling - more fine control could be provided by the application
		instance.accelerationStructureReference         = address;

		// The instance.transform value only contains 12 values, corresponding to a 4x3 matrix,
		// hence saving the last row that is anyway always (0,0,0,1).
		// Since the matrix is row-major, we simply copy the first 12 values of the original 4x4 matrix
		std::memcpy(&instance.transform, &transform, sizeof(instance.transform));

		return instance;
	}
}        // namespace maple