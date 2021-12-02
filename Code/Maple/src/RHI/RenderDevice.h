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

		virtual auto presentInternal() -> void                                                                                                                                         = 0;
		virtual auto presentInternal(CommandBuffer *commandBuffer) -> void                                                                                                             = 0;
		virtual auto drawIndexedInternal(CommandBuffer *commandBuffer, DrawType type, uint32_t count, uint32_t start = 0) const -> void                                                = 0;
		virtual auto drawInternal(CommandBuffer *commandBuffer, DrawType type, uint32_t count, DataType dataType = DataType::UnsignedInt, const void *indices = nullptr) const -> void = 0;

		virtual auto bindDescriptorSetsInternal(Pipeline *pipeline, CommandBuffer *commandBuffer, uint32_t dynamicOffset, const std::vector<std::shared_ptr<DescriptorSet>> &descriptorSets) -> void = 0;

		virtual auto clearRenderTarget(const std::shared_ptr<Texture> &texture, CommandBuffer *commandBuffer, const glm::vec4 &clearColor = {0.5f, 0.5f, 0.5f, 1.0f}) -> void
		{}

		virtual auto clearInternal(uint32_t bufferMask) -> void = 0;

		static auto clear(uint32_t bufferMask) -> void;
		static auto present() -> void;
		static auto present(CommandBuffer *commandBuffer) -> void;
		static auto bindDescriptorSets(Pipeline *pipeline, CommandBuffer *commandBuffer, uint32_t dynamicOffset, const std::vector<std::shared_ptr<DescriptorSet>> &descriptorSets) -> void;
		static auto draw(CommandBuffer *commandBuffer, DrawType type, uint32_t count, DataType datayType = DataType::UnsignedInt, const void *indices = nullptr) -> void;
		static auto drawIndexed(CommandBuffer *commandBuffer, DrawType type, uint32_t count, uint32_t start = 0) -> void;
		static auto create() -> std::shared_ptr<RenderDevice>;
	};
};        // namespace maple
