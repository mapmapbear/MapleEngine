//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "RHI/Pipeline.h"
#include "RHI/RenderDevice.h"
#include "VulkanSwapChain.h"

namespace maple
{
	class NativeWindow;
	class GLContext;
	class CommandBuffer;
	class Shader;

	class MAPLE_EXPORT VulkanRenderDevice : public RenderDevice
	{
	  public:
		VulkanRenderDevice();
		~VulkanRenderDevice();

	

		auto begin() -> void override;
		auto init() -> void override;
		auto onResize(uint32_t width, uint32_t height) -> void override;
		auto presentInternal() -> void override;
		auto presentInternal(const CommandBuffer *commandBuffer) -> void override;
		auto drawIndexedInternal(const CommandBuffer *commandBuffer, DrawType type, uint32_t count, uint32_t start) const -> void override;
		auto drawInternal(const CommandBuffer *commandBuffer, DrawType type, uint32_t count, DataType datayType, const void *indices) const -> void override;
		auto bindDescriptorSetsInternal(Pipeline *pipeline, const CommandBuffer *commandBuffer, uint32_t dynamicOffset, const std::vector<std::shared_ptr<DescriptorSet>> &sets) -> void override;
		auto clearRenderTarget(const std::shared_ptr<Texture> &texture, const CommandBuffer *commandBuffer, const glm::vec4 &clearColor) -> void override;
		auto dispatch(const CommandBuffer* commandBuffer, uint32_t x, uint32_t y, uint32_t z) -> void override;

		inline auto getDescriptorPool() const
		{
			return descriptorPool;
		}

	  protected:
		const std::string rendererName = "Vulkan-Renderer";

		uint32_t         currentSemaphoreIndex = 0;
		VkDescriptorPool descriptorPool;
		VkDescriptorSet  descriptorSetPool[16] = {};
	};
}        // namespace maple
