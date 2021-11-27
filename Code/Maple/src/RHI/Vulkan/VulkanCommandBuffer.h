//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanHelper.h"
#include "Engine/Interface/CommandBuffer.h"

namespace Maple
{
	class VulkanCommandBuffer;
	class VulkanRenderPass;
	class VulkanFrameBuffer;

	class VulkanCommandBuffer : public CommandBuffer
	{
	public:
		VulkanCommandBuffer(bool primary = true);
		~VulkanCommandBuffer();


		auto init(bool primary) -> bool override;
		auto execute(bool waitFence) -> void override;

		auto beginRecordingSecondary(RenderPass* renderPass, FrameBuffer* framebuffer) -> void override;
		auto executeSecondary(CommandBuffer* primaryCmdBuffer) -> void override;

		auto unload() -> void override;

		auto beginRecording() -> void override;
		auto endRecording() -> void override;

		auto updateViewport(uint32_t width, uint32_t height) -> void override;

		inline auto getCommandBuffer() { return &commandBuffer; }

		autoUnpack(commandBuffer);
		auto executeInternal(VkPipelineStageFlags flags, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore, bool waitFence) -> void;
	private:
		auto init(bool primary, VkCommandPool cmdPool) -> bool;
		VkCommandBuffer commandBuffer = nullptr;
		VkFence fence = nullptr;
		bool primary = false;
	};
};