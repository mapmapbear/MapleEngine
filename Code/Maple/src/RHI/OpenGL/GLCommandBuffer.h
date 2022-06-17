//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "RHI/CommandBuffer.h"

namespace maple
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
		auto beginRecordingSecondary(RenderPass* renderPass, FrameBuffer* framebuffer) -> void override;
		auto executeSecondary(const CommandBuffer* primaryCmdBuffer) -> void override;
		auto updateViewport(uint32_t width, uint32_t height) const -> void override {};
		auto bindPipeline(Pipeline* pipeline) -> void;
		auto unbindPipeline() -> void;
	private:
		bool primary = false;
		Pipeline* currentPipeline = nullptr;
	};
}        // namespace maple
