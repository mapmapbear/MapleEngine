//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "RHI/VertexBuffer.h"

namespace maple
{
	class GLVertexBuffer : public VertexBuffer
	{
	  public:
		explicit GLVertexBuffer(BufferUsage usage);
		~GLVertexBuffer();

		auto resize(uint32_t size) -> void override;
		auto setData(uint32_t size, const void *data) -> void override;
		auto setDataSub(uint32_t size, const void *data, uint32_t offset) -> void override;
		auto releasePointer() -> void override;
		auto bind(const CommandBuffer *commandBuffer, Pipeline *pipeline) -> void override;
		auto unbind() -> void override;
		auto getSize() -> uint32_t override
		{
			return size;
		}
		auto getPointerInternal() -> void * override;

	  private:
		BufferUsage usage;
		uint32_t    handle{};
		uint32_t    size   = 0;
		bool        mapped = false;
	};
}        // namespace maple
