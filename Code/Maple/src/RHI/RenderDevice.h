//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Engine/Core.h"
#include "RHI/Definitions.h"
#include <glm/glm.hpp>
#include <memory>

namespace maple
{
	class Texture;
	class CommandBuffer;
	class Pipeline;

	class MAPLE_EXPORT RenderDevice
	{
	  public:
		RenderDevice()                                                 = default;
		virtual ~RenderDevice()                                        = default;
		virtual auto init() -> void                                    = 0;
		virtual auto begin() -> void                                   = 0;
		virtual auto onResize(uint32_t width, uint32_t height) -> void = 0;
		virtual auto drawSplashScreen(const std::shared_ptr<Texture> &texture) -> void{};

		virtual auto setDepthTestingInternal(bool enabled) -> void{};
		virtual auto setStencilTestInternal(bool enabled) -> void{};
		virtual auto setStencilMaskInternal(uint32_t mask) -> void{};
		virtual auto setStencilOpInternal(StencilType fail, StencilType zfail, StencilType zpass) -> void{};
		virtual auto setStencilFunctionInternal(StencilType type, uint32_t ref, uint32_t mask) -> void{};

		virtual auto presentInternal() -> void{};
		virtual auto presentInternal(CommandBuffer *commandBuffer) -> void{};

		virtual auto dispatch(CommandBuffer *commandBuffer,uint32_t x,uint32_t y,uint32_t z) -> void{};

		virtual auto drawIndexedInternal(CommandBuffer *commandBuffer, DrawType type, uint32_t count, uint32_t start = 0) const -> void{};
		virtual auto drawInternal(CommandBuffer *commandBuffer, DrawType type, uint32_t count, DataType dataType = DataType::UnsignedInt, const void *indices = nullptr) const -> void{};
		virtual auto bindDescriptorSetsInternal(Pipeline *pipeline, CommandBuffer *commandBuffer, uint32_t dynamicOffset, const std::vector<std::shared_ptr<DescriptorSet>> &descriptorSets) -> void{};
		virtual auto clearRenderTarget(const std::shared_ptr<Texture> &texture, CommandBuffer *commandBuffer, const glm::vec4 &clearColor = {0.3f, 0.3f, 0.3f, 1.0f}) -> void{};
		virtual auto clearInternal(uint32_t bufferMask) -> void{};

		static auto clear(uint32_t bufferMask) -> void;
		static auto present() -> void;
		static auto present(CommandBuffer *commandBuffer) -> void;
		static auto bindDescriptorSets(Pipeline *pipeline, CommandBuffer *commandBuffer, uint32_t dynamicOffset, const std::vector<std::shared_ptr<DescriptorSet>> &descriptorSets) -> void;
		static auto draw(CommandBuffer *commandBuffer, DrawType type, uint32_t count, DataType datayType = DataType::UnsignedInt, const void *indices = nullptr) -> void;
		static auto drawIndexed(CommandBuffer *commandBuffer, DrawType type, uint32_t count, uint32_t start = 0) -> void;
		static auto setStencilOp(StencilType fail, StencilType zfail, StencilType zpass) -> void;
		static auto setStencilFunction(StencilType type, uint32_t ref, uint32_t mask) -> void;
		static auto setStencilMask(uint32_t mask) -> void;
		static auto setStencilTest(bool enable) -> void;
		static auto setDepthTest(bool enable) -> void;
		static auto create() -> std::shared_ptr<RenderDevice>;
	};
};        // namespace maple
