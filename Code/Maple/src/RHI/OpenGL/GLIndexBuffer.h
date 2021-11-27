#pragma once

#include "RHI/IndexBuffer.h"

namespace Maple
{
	class GLIndexBuffer : public IndexBuffer
	{
	  public:
		GLIndexBuffer(const uint16_t *data, uint32_t count, BufferUsage bufferUsage);
		GLIndexBuffer(const uint32_t *data, uint32_t count, BufferUsage bufferUsage);
		~GLIndexBuffer();

		auto bind(CommandBuffer *commandBuffer) const -> void override;
		auto unbind() const -> void override;
		auto getCount() const -> uint32_t override;

		auto getPointerInternal() -> void * override;
		auto releasePointer() -> void override;

		inline auto setCount(uint32_t indexCount) -> void override
		{
			count = indexCount;
		};

	  private:
		uint32_t    handle = 0;
		uint32_t    count  = 0;
		BufferUsage usage  = BufferUsage::Dynamic;
		bool        mapped = false;
	};
}        // namespace Maple
