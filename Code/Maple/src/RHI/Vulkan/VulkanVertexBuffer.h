
//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "RHI/VertexBuffer.h"
#include "VulkanBuffer.h"
#include <memory>

namespace maple
{
	class VulkanVertexBuffer : public VulkanBuffer, public VertexBuffer
	{
	  public:
		VulkanVertexBuffer(const BufferUsage &usage);
		~VulkanVertexBuffer();

		auto bind(const CommandBuffer *commandBuffer, Pipeline *pipeline) -> void override;
		auto resize(uint32_t size) -> void override;
		auto setData(uint32_t size, const void *data) -> void override;
		auto setDataSub(uint32_t size, const void *data, uint32_t offset) -> void override;
		auto releasePointer() -> void override;
		auto unbind() -> void override;

		inline auto getSize() -> uint32_t override
		{
			return size;
		}
		auto getPointerInternal() -> void * override;

	  protected:
		bool        mappedBuffer = false;
		BufferUsage bufferUsage  = BufferUsage::Static;
	};
};        // namespace maple
