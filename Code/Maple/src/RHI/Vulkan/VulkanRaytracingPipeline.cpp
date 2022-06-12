//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "VulkanRaytracingPipeline.h"
#include "VulkanCommandBuffer.h"
#include "VulkanContext.h"
#include "VulkanDescriptorSet.h"
#include "VulkanDevice.h"
#include "VulkanFrameBuffer.h"
#include "VulkanRenderPass.h"
#include "VulkanShader.h"
#include "VulkanSwapChain.h"
#include "VulkanTexture.h"

#include "Engine/Vertex.h"
#include "Others/Console.h"

#include <memory>

#include "Application.h"

namespace maple
{
	VulkanRaytracingPipeline::VulkanRaytracingPipeline(const PipelineInfo& info)
	{
		init(info);
	}

	auto VulkanRaytracingPipeline::init(const PipelineInfo& info) -> bool
	{
		PROFILE_FUNCTION();
		shader = info.shader;
		auto vkShader = std::static_pointer_cast<VulkanShader>(info.shader);
		description = info;
		pipelineLayout = vkShader->getPipelineLayout();

		VkRayTracingPipelineCreateInfoKHR pipelineCreateInfo{};

		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		pipelineCreateInfo.layout = pipelineLayout;
		pipelineCreateInfo.flags = 0;
		pipelineCreateInfo.pStages = vkShader->getShaderStages().data();
		pipelineCreateInfo.stageCount = vkShader->getShaderStages().size();
		pipelineCreateInfo.pGroups = vkShader->getShaderGroups().data();
		pipelineCreateInfo.groupCount = vkShader->getShaderGroups().size();

		VK_CHECK_RESULT(vkCreateRayTracingPipelinesKHR(*VulkanDevice::get(), VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline));

		return true;
	}

	auto VulkanRaytracingPipeline::bind(const CommandBuffer* cmdBuffer, uint32_t layer, int32_t cubeFace, int32_t mipMapLevel) -> FrameBuffer*
	{
		PROFILE_FUNCTION();
		vkCmdBindPipeline(static_cast<const VulkanCommandBuffer*>(cmdBuffer)->getCommandBuffer(), 
			VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
			pipeline);
		return nullptr;
	}
};        // namespace maple
