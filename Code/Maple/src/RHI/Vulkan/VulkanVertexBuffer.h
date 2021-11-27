
//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <memory>
#include "VulkanBuffer.h"
#include "Engine/Interface/VertexBuffer.h"


namespace Maple
{
	class CommandBuffer;
	class Pipeline;
   /**
	* VertexBuffer for Vulkan
	*/
	class VulkanVertexBuffer : public VulkanBuffer,public VertexBuffer
	{
	public:
		VulkanVertexBuffer();
		VulkanVertexBuffer(const BufferUsage & usage);
		~VulkanVertexBuffer();


		/**
		 * bind to cmd 
		 */
		auto bind(CommandBuffer* commandBuffer, Pipeline* pipeline) -> void;
		auto unbind() -> void;


		auto resize(uint32_t size) -> void override;
		auto setData(uint32_t size, const void* data) -> void override;
		auto setDataSub(uint32_t size, const void* data, uint32_t offset) -> void override;
		auto releasePointer() -> void override;
		auto unbind() -> void override;
		auto getSize()->uint32_t override { return size; } 
		auto getPointerInternal() -> void* override;



	protected:
		bool mappedBuffer = false;
		BufferUsage bufferUsage = BufferUsage::STATIC;
	};
};