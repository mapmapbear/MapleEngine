//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "BufferLayout.h"
#include "Definitions.h"
#include "Shader.h"
#include <array>
#include <memory>

namespace maple
{
	struct PipelineInfo
	{
		std::shared_ptr<Shader> shader;

		bool transparencyEnabled = true;
		bool depthBiasEnabled    = false;
		bool swapChainTarget     = false;
		bool clearTargets        = false;
		bool stencilTest         = false;
		bool depthTest           = true;

		uint32_t    stencilMask = 0x00;
		StencilType stencilFunc = StencilType::Always;

		StencilType stencilFail      = StencilType::Keep;
		StencilType stencilDepthFail = StencilType::Keep;
		StencilType stencilDepthPass = StencilType::Replace;

		CullMode    cullMode    = CullMode::Back;
		PolygonMode polygonMode = PolygonMode::Fill;
		DrawType    drawType    = DrawType::Triangle;
		BlendMode   blendMode   = BlendMode::None;
		glm::vec4   clearColor  = {0.2f, 0.2f, 0.2f, 1.0};

		std::shared_ptr<Texture> depthTarget      = nullptr;
		std::shared_ptr<Texture> depthArrayTarget = nullptr;

		std::array<std::shared_ptr<Texture>, MAX_RENDER_TARGETS> colorTargets = {};
	};

	class MAPLE_EXPORT Pipeline
	{
	  public:
		//static auto create(const PipelineInfo &pipelineDesc) -> std::shared_ptr<Pipeline>;
		static auto get(const PipelineInfo &pipelineDesc) -> std::shared_ptr<Pipeline>;

		virtual ~Pipeline() = default;

		virtual auto getWidth() -> uint32_t                                                                                         = 0;
		virtual auto getHeight() -> uint32_t                                                                                        = 0;
		virtual auto getShader() const -> std::shared_ptr<Shader>                                                                   = 0;
		virtual auto bind(CommandBuffer *commandBuffer, uint32_t layer = 0, int32_t cubeFace = -1, int32_t mipMapLevel = 0) -> void = 0;
		virtual auto end(CommandBuffer *commandBuffer) -> void = 0;
		virtual auto clearRenderTargets(CommandBuffer *commandBuffer) -> void{};

	  protected:
		PipelineInfo description;
	};
}        // namespace maple
