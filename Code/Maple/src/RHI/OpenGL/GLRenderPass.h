//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "RHI/RenderPass.h"

namespace maple
{
	class GLRenderPass : public RenderPass
	{
	  public:
		GLRenderPass(const RenderPassInfo &renderPassDesc);
		~GLRenderPass();

		virtual auto getAttachmentCount() const -> int32_t override
		{
			return clearCount;
		}
		virtual auto beginRenderPass(const CommandBuffer *commandBuffer, const glm::vec4 &clearColor, FrameBuffer *frame, SubPassContents contents, uint32_t width, uint32_t height, int32_t cubeFace = -1, int32_t mipMapLevel = 0) const -> void override;
		virtual auto endRenderPass(const CommandBuffer *commandBuffer) -> void override;

	  private:
		bool    clear      = true;
		int32_t clearCount = 0;
	};
}        // namespace maple
