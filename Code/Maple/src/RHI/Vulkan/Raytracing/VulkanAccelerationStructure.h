//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "BottomLevelGeometry.h"
#include "RHI/Vulkan/Vk.h"
#include "RHI/AccelerationStructure.h"

#include "Engine/Core.h"
#include <glm/glm.hpp>

namespace maple
{
	class RayTracingProperties;
	class BottomLevelGeometry;

	class VulkanAccelerationStructure : public AccelerationStructure
	{
	  public:
		NO_COPYABLE(VulkanAccelerationStructure);

		virtual ~VulkanAccelerationStructure();

		inline auto &getBuildSizes() const
		{
			return buildSizesInfo;
		}

		virtual auto getBuildScratchSize() const -> uint64_t override
		{
			return buildSizesInfo.buildScratchSize;
		}

		static auto memoryBarrier(VkCommandBuffer cmdBuffer) -> void;

		inline auto getAccelerationStructure() const
		{
			return accelerationStructure;
		}

	  protected:
		explicit VulkanAccelerationStructure(const std::shared_ptr<RayTracingProperties> &rayTracingProperties);
		auto getBuildSizes(const uint32_t *maxPrimitiveCounts) const -> const VkAccelerationStructureBuildSizesInfoKHR;
		auto createAccelerationStructure(const VulkanBuffer::Ptr &buffer, VkDeviceSize resultOffset) -> void;

		VkBuildAccelerationStructureFlagsKHR        flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{};
		VkAccelerationStructureBuildSizesInfoKHR    buildSizesInfo{};

		VkAccelerationStructureKHR            accelerationStructure = nullptr;
		std::shared_ptr<RayTracingProperties> rayTracingProperties;
	};

	class BottomLevelAccelerationStructure final : public VulkanAccelerationStructure
	{
	  public:
		NO_COPYABLE(BottomLevelAccelerationStructure);

		BottomLevelAccelerationStructure(const std::shared_ptr<RayTracingProperties> &properties, const BottomLevelGeometry &geometries);

		~BottomLevelAccelerationStructure();

		auto generate(
		    VkCommandBuffer          commandBuffer,
		    const VulkanBuffer::Ptr &scratchBuffer,
		    VkDeviceSize             scratchOffset,
		    const VulkanBuffer::Ptr &resultBuffer,
		    VkDeviceSize             resultOffset) -> void;

	  private:
		BottomLevelGeometry geometries;
	};

	class TopLevelAccelerationStructure final : public VulkanAccelerationStructure
	{
	  public:
		NO_COPYABLE(TopLevelAccelerationStructure);

		TopLevelAccelerationStructure(const std::shared_ptr<RayTracingProperties> &rayTracingProperties, VkDeviceAddress instanceAddress, uint32_t instancesCount);
		virtual ~TopLevelAccelerationStructure();

		auto generate(
		    VkCommandBuffer          commandBuffer,
		    const VulkanBuffer::Ptr &scratchBuffer,
		    VkDeviceSize             scratchOffset,
		    const VulkanBuffer::Ptr &resultBuffer,
		    VkDeviceSize             resultOffset) -> void;

		static auto createInstance(
		    const BottomLevelAccelerationStructure &bottomLevelAs,
		    const glm::mat4 &                       transform,
		    uint32_t                                instanceId,
		    uint32_t                                hitGroupId) -> VkAccelerationStructureInstanceKHR;

	  private:
		uint32_t                                        instancesCount;
		VkAccelerationStructureGeometryInstancesDataKHR instancesVk{};
		VkAccelerationStructureGeometryKHR              topASGeometry{};
	};

}        // namespace maple