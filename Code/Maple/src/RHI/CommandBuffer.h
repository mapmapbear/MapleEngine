//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <cstdint>
#include <memory>

namespace Maple
{
	class RenderPass;
	class FrameBuffer;
	class Pipeline;

	class CommandBuffer
	{
	  public:
		virtual ~CommandBuffer() = default;

		static auto create() -> std::shared_ptr<CommandBuffer>;

		virtual auto init(bool primary) -> bool                                                        = 0;
		virtual auto unload() -> void                                                                  = 0;
		virtual auto beginRecording() -> void                                                          = 0;
		virtual auto beginRecordingSecondary(RenderPass *renderPass, FrameBuffer *framebuffer) -> void = 0;
		virtual auto endRecording() -> void                                                            = 0;
		virtual auto executeSecondary(CommandBuffer *primaryCmdBuffer) -> void                         = 0;
		virtual auto updateViewport(uint32_t width, uint32_t height) -> void                           = 0;
		virtual auto flush() -> bool
		{
			return true;
		}
		virtual auto bindPipeline(Pipeline *pipeline) -> void = 0;
		virtual auto unbindPipeline() -> void                 = 0;
	};
}        // namespace Maple
