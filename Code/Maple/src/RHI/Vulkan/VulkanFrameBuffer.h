//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////


#pragma once

#include <memory>
#include "Engine/Interface/FrameBuffer.h"
#include "VulkanHelper.h"
namespace maple
{
	
	class VulkanFrameBuffer : public FrameBuffer
	{
	public:
		VulkanFrameBuffer(const FrameBufferInfo& info);
		~VulkanFrameBuffer();
		autoUnpack(buffer);

		inline auto& getFrameBufferInfo() const { return info; }

	private:
		FrameBufferInfo info;
		VkFramebuffer buffer = nullptr;
	};


};