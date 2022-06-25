//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "RHI/BatchTask.h"
#include "RHI/IndexBuffer.h"
#include "RHI/VertexBuffer.h"
#include <glm/glm.hpp>
#include <memory>

namespace maple
{
	class StorageBuffer;

	class AccelerationStructure
	{
	  public:
		using Ptr = std::shared_ptr<AccelerationStructure>;

		static auto createTopLevel(const uint32_t maxInstanceCount) -> Ptr;

		static auto createBottomLevel(const VertexBuffer::Ptr &vertexBuffer, const IndexBuffer::Ptr &indexBuffer, uint32_t vertexCount, BatchTask::Ptr tasks) -> Ptr;

		virtual auto getBuildScratchSize() const -> uint64_t = 0;

		virtual auto updateTLAS(void *buffer, const glm::mat3x4 &transform, uint32_t instanceId, uint64_t instanceAddress) -> void = 0;

		virtual auto getDeviceAddress() const -> uint64_t = 0;

		virtual auto mapHost() -> void * = 0;

		virtual auto unmap() -> void = 0;
	};

	class NullAccelerationStructure : public AccelerationStructure
	{
	  public:
		virtual auto getBuildScratchSize() const -> uint64_t
		{
			return 0;
		}

		virtual auto updateTLAS(void *buffer, const glm::mat3x4 &transform, uint32_t instanceId, uint64_t instanceAddress) -> void
		{}

		virtual auto getDeviceAddress() const -> uint64_t
		{
			return 0;
		}

		virtual auto mapHost() -> void *
		{
			return nullptr;
		}

		virtual auto unmap() -> void
		{
		}
	};        // namespace maple

}        // namespace maple