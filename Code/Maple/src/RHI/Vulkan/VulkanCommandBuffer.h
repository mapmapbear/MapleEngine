//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "RHI/CommandBuffer.h"
#include "VulkanHelper.h"
#include "Engine/Core.h"

namespace maple
{
	enum class CommandBufferState : uint8_t
	{
		Idle,
		Recording,
		Ended,
		Submitted
	};

	class VulkanCommandBuffer : public CommandBuffer
	{
	  public:
		VulkanCommandBuffer();
		VulkanCommandBuffer(VkCommandBuffer commandBuffer);
		~VulkanCommandBuffer();

		NO_COPYABLE(VulkanCommandBuffer);

		auto init(bool primary) -> bool override;
		auto init(bool primary, VkCommandPool commandPool) -> bool;
		auto unload() -> void override;
		auto beginRecording() -> void override;
		auto beginRecordingSecondary(RenderPass *renderPass, FrameBuffer *framebuffer) -> void override;
		auto endRecording() -> void override;
		auto executeSecondary(CommandBuffer *primaryCmdBuffer) -> void override;
		auto updateViewport(uint32_t width, uint32_t height) -> void override;
		auto bindPipeline(Pipeline *pipeline) -> void override;
		auto unbindPipeline() -> void override;
		auto flush() -> bool override;

		inline auto isRecording() const -> bool override
		{
			return state == CommandBufferState::Recording;
		}

		inline auto getState() const
		{
			return state;
		}

		inline auto getSemaphore() const
		{
			return semaphore;
		}

		auto wait() -> void;
		auto reset() -> void;

		auto executeInternal(VkPipelineStageFlags flags, VkSemaphore signalSemaphore, bool waitFence) -> void;

		inline auto getCommandBuffer() const
		{
			return commandBuffer;
		}

	  private:
		VkCommandBuffer              commandBuffer;
		VkCommandPool                commandPool   = nullptr;
		bool                         primary;
		CommandBufferState           state = CommandBufferState::Idle;
		std::shared_ptr<VulkanFence> fence;
		VkSemaphore                  semaphore = nullptr;

		Pipeline *  boundPipeline   = nullptr;
		RenderPass *boundRenderPass = nullptr;
	};
};        // namespace maple
