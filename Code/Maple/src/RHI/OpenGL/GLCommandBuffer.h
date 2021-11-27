#pragma once

#include "RHI/CommandBuffer.h"

namespace Maple
{
	class GLCommandBuffer : public CommandBuffer
	{
	  public:
		GLCommandBuffer();
		~GLCommandBuffer();

		auto init(bool primary) -> bool override;
		auto beginRecording() -> void override;
		auto endRecording() -> void override;
		auto unload() -> void override;
		auto beginRecordingSecondary(RenderPass *renderPass, FrameBuffer *framebuffer) -> void override;
		auto executeSecondary(CommandBuffer *primaryCmdBuffer) -> void override;

		auto updateViewport(uint32_t width, uint32_t height) -> void override{};
		auto bindPipeline(Pipeline *pipeline) -> void;
		auto unbindPipeline() -> void;

	  private:
		bool primary = false;
		Pipeline * currentPipeline = nullptr;

	};
}        // namespace Maple
