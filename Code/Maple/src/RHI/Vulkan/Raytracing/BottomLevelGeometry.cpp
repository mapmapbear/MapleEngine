//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "BottomLevelGeometry.h"
#include "Engine/Vertex.h"

namespace maple
{
	auto BottomLevelGeometry::addGeometryTriangles(const VulkanBuffer::Ptr& vertexBuffer, const VulkanBuffer::Ptr& indexBuffer, uint32_t vertexOffset, uint32_t vertexCount, uint32_t indexOffset, uint32_t indexCount, bool isOpaque) -> void
	{
		VkAccelerationStructureGeometryKHR geometry = {};
		geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		geometry.pNext = nullptr;
		geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		geometry.geometry.triangles.pNext = nullptr;

		geometry.geometry.triangles.vertexData.deviceAddress = vertexBuffer->getDeviceAddress();

		geometry.geometry.triangles.vertexStride = sizeof(Vertex);
		geometry.geometry.triangles.maxVertex = vertexCount;
		geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		geometry.geometry.triangles.indexData.deviceAddress = indexBuffer->getDeviceAddress();
		geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
		geometry.geometry.triangles.transformData = {};
		geometry.flags = isOpaque ? VK_GEOMETRY_OPAQUE_BIT_KHR : 0;

		VkAccelerationStructureBuildRangeInfoKHR buildOffsetInfo = {};
		buildOffsetInfo.firstVertex = vertexOffset / sizeof(Vertex);
		buildOffsetInfo.primitiveOffset = indexOffset;
		buildOffsetInfo.primitiveCount = indexCount / 3;
		buildOffsetInfo.transformOffset = 0;

		geometries.emplace_back(geometry);
		buildOffsetInfos.emplace_back(buildOffsetInfo);
	}

	auto BottomLevelGeometry::addGeometryAabb(const VulkanBuffer::Ptr& aabbBuffer, uint32_t aabbOffset, uint32_t aabbCount, bool isOpaque) -> void
	{
		VkAccelerationStructureGeometryKHR geometry = {};
		geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		geometry.pNext = nullptr;
		geometry.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
		geometry.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
		geometry.geometry.aabbs.pNext = nullptr;
		geometry.geometry.aabbs.data.deviceAddress = aabbBuffer->getDeviceAddress();
		geometry.geometry.aabbs.stride = sizeof(VkAabbPositionsKHR);
		geometry.flags = isOpaque ? VK_GEOMETRY_OPAQUE_BIT_KHR : 0;

		VkAccelerationStructureBuildRangeInfoKHR buildOffsetInfo = {};
		buildOffsetInfo.firstVertex = 0;
		buildOffsetInfo.primitiveOffset = aabbOffset;
		buildOffsetInfo.primitiveCount = aabbCount;
		buildOffsetInfo.transformOffset = 0;

		geometries.emplace_back(geometry);
		buildOffsetInfos.emplace_back(buildOffsetInfo);
	}

}