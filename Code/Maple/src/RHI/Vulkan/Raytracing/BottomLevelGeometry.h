//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Engine/Core.h"
#include "RHI/Vulkan/Vk.h"
#include "RHI/Vulkan/VulkanBuffer.h"
#include <vector>

namespace maple
{
	class BottomLevelGeometry final
	{
	  public:
		inline auto getSize() const
		{
			return geometries.size();
		}
		inline auto &getGeometry() const
		{
			return geometries;
		}
		inline auto &getBuildOffsetInfo() const
		{
			return buildOffsetInfos;
		}

		auto addGeometryTriangles(
		    const VulkanBuffer::Ptr &vertexBuffer,
		    const VulkanBuffer::Ptr &indexBuffer,
		    uint32_t                 vertexOffset,
		    uint32_t                 vertexCount,
		    uint32_t                 indexOffset,
		    uint32_t                 indexCount,
		    bool                     isOpaque) -> void;

		auto addGeometryAabb(
		    const VulkanBuffer::Ptr &aabbBuffer,
		    uint32_t                 aabbOffset,
		    uint32_t                 aabbCount,
		    bool                     isOpaque) -> void;

	  private:
		// The geometry to build, addresses of vertices and indices.
		std::vector<VkAccelerationStructureGeometryKHR> geometries;
		// the number of elements to build and offsets
		std::vector<VkAccelerationStructureBuildRangeInfoKHR> buildOffsetInfos;
	};
}        // namespace maple