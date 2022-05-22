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
		VulkanCommandBuffer(CommandBufferType cmdBufferType = CommandBufferType::Graphics);
		VulkanCommandBuffer(VkCommandBuffer commandBuffer, CommandBufferType cmdBufferType = CommandBufferType::Graphics);
		~VulkanCommandBuffer();

		NO_COPYABLE(VulkanCommandBuffer);

		auto init(bool primary) -> bool override;
		auto init(bool primary, VkCommandPool commandPool) -> bool;
		auto unload() -> void override;
		auto beginRecording() -> void override;
		auto beginRecordingSecondary(RenderPass* renderPass, FrameBuffer* framebuffer) -> void override;
		auto endRecording() -> void override;
		auto executeSecondary(const CommandBuffer* primaryCmdBuffer) -> void override;
		auto updateViewport(uint32_t width, uint32_t height) const -> void override;
		auto bindPipeline(Pipeline* pipeline) -> void override;
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

		auto wait() -> void;
		auto reset() -> void;


		auto executeInternal(
			const std::vector<VkPipelineStageFlags>& flags,
			const std::vector<VkSemaphore>& waitSemaphores,
			const std::vector<VkSemaphore>& signalSemaphores,
			bool waitFence) -> void;


		inline auto getCommandBuffer() const
		{
			return commandBuffer;
		}

		inline auto getCommandBuffeType() const { return cmdBufferType; }

		auto addTask(const std::function<void(const CommandBuffer*)>& task)->void override;
	private:
		VkCommandBuffer              commandBuffer = nullptr;
		VkCommandPool                commandPool = nullptr;
		bool                         primary;

		CommandBufferState           state = CommandBufferState::Idle;

		Pipeline* boundPipeline = nullptr;
		RenderPass* boundRenderPass = nullptr;

		CommandBufferType cmdBufferType = CommandBufferType::Graphics;

		std::vector< std::function<void(const CommandBuffer*)>> tasks;
	};
};        // namespace maple
