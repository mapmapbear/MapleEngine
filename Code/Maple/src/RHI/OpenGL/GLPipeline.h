#pragma once

#include "RHI/Pipeline.h"

namespace maple
{
	class GLRenderPass;
	class CommandBuffer;
	class RenderPass;
	class FrameBuffer;

	class GLPipeline : public Pipeline
	{
	  public:
		GLPipeline(const PipelineInfo &pipelineDesc);
		~GLPipeline();

		auto init(const PipelineInfo &pipelineDesc) -> bool;
		auto end(CommandBuffer *commandBuffer) -> void override;
		auto bind(CommandBuffer *commandBuffer, uint32_t layer = 0, int32_t cubeFace = -1, int32_t mipMapLevel = 0) -> void override;
		auto clearRenderTargets(CommandBuffer *commandBuffer) -> void override;
		auto bindVertexArray() -> void;
		auto createFrameBuffers() -> void;
		auto getWidth() -> uint32_t override;
		auto getHeight() -> uint32_t override;

		inline auto getShader() const -> std::shared_ptr<Shader> override
		{
			return shader;
		}

	  private:
		std::shared_ptr<Shader>                   shader;
		std::shared_ptr<RenderPass>               renderPass;
		std::vector<std::shared_ptr<FrameBuffer>> frameBuffers;

		std::string  pipelineName;
		bool         transparencyEnabled = false;
		uint32_t     vertexArray         = -1;
		BufferLayout vertexBufferLayout;
	};
}        // namespace maple
