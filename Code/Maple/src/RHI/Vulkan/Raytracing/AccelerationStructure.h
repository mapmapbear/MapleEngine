//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "RHI/Vulkan/Vk.h"
#include "BottomLevelGeometry.h"

#include "Engine/Core.h"
#include <glm/glm.hpp>

namespace maple
{
	class RayTracingProperties;
	class BottomLevelGeometry;

	class AccelerationStructure
	{
	public:
		NO_COPYABLE(AccelerationStructure);

		virtual ~AccelerationStructure();

		inline auto& getBuildSizes() const { return buildSizesInfo; }

		static auto memoryBarrier(VkCommandBuffer cmdBuffer) -> void;

		inline auto getAccelerationStructure() const { return accelerationStructure; }

	protected:

		explicit AccelerationStructure(const RayTracingProperties& rayTracingProperties);
		auto getBuildSizes(const uint32_t* maxPrimitiveCounts) const ->const VkAccelerationStructureBuildSizesInfoKHR;
		auto createAccelerationStructure(const VulkanBuffer::Ptr& buffer, VkDeviceSize resultOffset) -> void;

		VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{};
		VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo{};

		VkAccelerationStructureKHR accelerationStructure = nullptr;
		const RayTracingProperties& rayTracingProperties;
	};

	class BottomLevelAccelerationStructure final : public AccelerationStructure
	{
	public:
		NO_COPYABLE(BottomLevelAccelerationStructure);

		BottomLevelAccelerationStructure(const class RayTracingProperties& rayTracingProperties, const BottomLevelGeometry& geometries);

		~BottomLevelAccelerationStructure();

		auto generate(
			VkCommandBuffer commandBuffer,
			const VulkanBuffer::Ptr& scratchBuffer,
			VkDeviceSize scratchOffset,
			const VulkanBuffer::Ptr& resultBuffer,
			VkDeviceSize resultOffset) -> void;

	private:

		BottomLevelGeometry geometries;
	};

	class TopLevelAccelerationStructure final : public AccelerationStructure
	{
	public:
		NO_COPYABLE(TopLevelAccelerationStructure);

		TopLevelAccelerationStructure(const RayTracingProperties& rayTracingProperties,VkDeviceAddress instanceAddress,uint32_t instancesCount);
		virtual ~TopLevelAccelerationStructure();

		auto generate(
			VkCommandBuffer commandBuffer,
			const VulkanBuffer::Ptr& scratchBuffer,
			VkDeviceSize scratchOffset,
			const VulkanBuffer::Ptr& resultBuffer,
			VkDeviceSize resultOffset) -> void;

		static auto createInstance(
			const BottomLevelAccelerationStructure& bottomLevelAs,
			const glm::mat4& transform,
			uint32_t instanceId,
			uint32_t hitGroupId) ->VkAccelerationStructureInstanceKHR;

	private:

		uint32_t instancesCount;
		VkAccelerationStructureGeometryInstancesDataKHR instancesVk{};
		VkAccelerationStructureGeometryKHR topASGeometry{};
	};

}