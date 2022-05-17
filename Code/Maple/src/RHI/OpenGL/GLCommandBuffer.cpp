#include "GLCommandBuffer.h"
#include "RHI/Pipeline.h"

namespace maple
{
	GLCommandBuffer::GLCommandBuffer()
	{
	}

	GLCommandBuffer::~GLCommandBuffer()
	{
	}

	auto GLCommandBuffer::init(bool primary) -> bool
	{
		return true;
	}

	auto GLCommandBuffer::unload() -> void
	{
	}

	auto GLCommandBuffer::beginRecording() -> void
	{
	}

	auto GLCommandBuffer::beginRecordingSecondary(RenderPass *renderPass, FrameBuffer *framebuffer) -> void
	{
	}

	auto GLCommandBuffer::endRecording() -> void
	{
	}

	auto GLCommandBuffer::executeSecondary(const CommandBuffer *primaryCmdBuffer) -> void
	{
	}

	auto GLCommandBuffer::bindPipeline(Pipeline *pipeline) -> void
	{
		if (pipeline != currentPipeline)
		{
			pipeline->bind(this);
			currentPipeline = pipeline;
		}
	}

	auto GLCommandBuffer::unbindPipeline() -> void
	{
		if (currentPipeline)
			currentPipeline->end(this);
		currentPipeline = nullptr;
	}
}        // namespace maple
