//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "GLSwapChain.h"
#include "RHI/Pipeline.h"
#include "RHI/RenderDevice.h"

namespace maple
{
	class GLContext;
	class CommandBuffer;
	class Shader;

	class MAPLE_EXPORT GLRenderDevice : public RenderDevice
	{
	  public:
		GLRenderDevice();
		~GLRenderDevice();

		auto begin() -> void override;
		auto init() -> void override;
		auto onResize(uint32_t width, uint32_t height) -> void override;
		auto presentInternal() -> void override;
		auto presentInternal(const CommandBuffer *commandBuffer) -> void override;
		auto drawArraysInternal(const CommandBuffer* commandBuffer, DrawType type, uint32_t count, uint32_t start = 0) const -> void override;
		auto drawIndexedInternal(const CommandBuffer *commandBuffer, DrawType type, uint32_t count, uint32_t start) const -> void override;
		auto drawInternal(const CommandBuffer *commandBuffer, DrawType type, uint32_t count, DataType datayType, const void *indices) const -> void override;
		auto bindDescriptorSetsInternal(Pipeline *pipeline, const CommandBuffer *commandBuffer, uint32_t dynamicOffset, const std::vector<std::shared_ptr<DescriptorSet>> &sets) -> void override;

		auto setRenderModeInternal(RenderMode mode) -> void;
		auto setDepthTestingInternal(bool enabled) -> void;
		auto setBlendInternal(bool enabled) -> void;

		auto setStencilTestInternal(bool enabled) -> void override;
		
		auto setCullingInternal(bool enabled, bool front) -> void;
		auto setDepthMaskInternal(bool enabled) -> void;
		auto setViewportInternal(uint32_t x, uint32_t y, uint32_t width, uint32_t height) -> void;
		auto setPixelPackType(PixelPackType type) -> void;
		auto setColorMaskInternal(bool r, bool g, bool b, bool a) -> void;
		auto setBlendFunctionInternal(RendererBlendFunction source, RendererBlendFunction destination) -> void;
		auto setBlendEquationInternal(RendererBlendFunction blendEquation) -> void;

		auto setStencilMaskInternal(uint32_t mask) -> void override;
		auto setStencilFunctionInternal(StencilType type, uint32_t ref, uint32_t mask) -> void override;
		auto setStencilOpInternal(StencilType fail, StencilType zfail, StencilType zpass) -> void override;

		auto clearRenderTarget(const std::shared_ptr<Texture> &texture, const CommandBuffer *commandBuffer, const glm::vec4 &clearColor) -> void override;

		auto clearInternal(uint32_t bufferMask) -> void override;
		auto dispatch(const CommandBuffer *commandBuffer, uint32_t x, uint32_t y, uint32_t z) -> void override;
		auto memoryBarrier(const CommandBuffer* commandBuffer, uint32_t flag) -> void override;

	  protected:
		const std::string rendererName = "OpenGL-Renderer";
	};
}        // namespace maple
