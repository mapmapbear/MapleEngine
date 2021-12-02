//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "VulkanDescriptorSet.h"
#include "VulkanHelper.h"
#include "VulkanDevice.h"
#include "VulkanPipeline.h"
#include "Others/Console.h"
#include "VulkanBuffer.h"
#include "VulkanTexture.h"
#include "VulkanUniformBuffer.h"
#include "Engine/Profiler.h"
#include <array>

#define MAX_BUFFER_INFOS 16
#define MAX_IMAGE_INFOS 16
#define MAX_WRITE_DESCTIPTORS 16

namespace maple
{

	VulkanDescriptorSet::VulkanDescriptorSet(const DescriptorCreateInfo& info)
	{
		PROFILE_FUNCTION();

		m_FramesInFlight = uint32_t(VKRenderer::GetMainSwapChain()->GetSwapChainBufferCount());

		

		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.descriptorPool = VKRenderer::GetRenderer()->GetDescriptorPool();
		descriptorSetAllocateInfo.pSetLayouts = static_cast<Graphics::VKShader*>(descriptorDesc.shader)->GetDescriptorLayout(descriptorDesc.layoutIndex);
		descriptorSetAllocateInfo.descriptorSetCount = descriptorDesc.count;
		descriptorSetAllocateInfo.pNext = nullptr;

		m_Shader = info.shader;
		m_Descriptors = m_Shader->GetDescriptorInfo(descriptorDesc.layoutIndex);

		for (auto& descriptor : m_Descriptors.descriptors)
		{
			if (descriptor.type == DescriptorType::UNIFORM_BUFFER)
			{
				for (uint32_t frame = 0; frame < m_FramesInFlight; frame++)
				{
					//Uniform Buffer per frame in flight
					auto buffer = SharedPtr<Graphics::UniformBuffer>(Graphics::UniformBuffer::Create());
					buffer->Init(descriptor.size, nullptr);
					m_UniformBuffers[frame][descriptor.name] = buffer;
				}

				Buffer localStorage;
				localStorage.Allocate(descriptor.size);
				localStorage.InitialiseEmpty();

				UniformBufferInfo info;
				info.LocalStorage = localStorage;
				info.HasUpdated[0] = false;
				info.HasUpdated[1] = false;
				info.HasUpdated[2] = false;

				info.m_Members = descriptor.m_Members;
				m_UniformBuffersData[descriptor.name] = info;
			}
		}

		for (uint32_t frame = 0; frame < m_FramesInFlight; frame++)
		{
			m_DescriptorDirty[frame] = true;
			m_DescriptorSet[frame] = nullptr;
			VK_CHECK_RESULT(vkAllocateDescriptorSets(VKDevice::GetHandle(), &descriptorSetAllocateInfo, &m_DescriptorSet[frame]));
		}
	}

	auto VulkanDescriptorSet::update() -> void 
	{
		
	}

	auto VulkanDescriptorSet::setDynamicOffset(uint32_t offset) -> void 
	{
	}

	auto VulkanDescriptorSet::getDynamicOffset() const -> uint32_t 
	{
	}

	auto VulkanDescriptorSet::setTexture(const std::string& name, const std::vector<std::shared_ptr<Texture>>& textures, TextureType textureType) -> void 
	{
	}

	auto VulkanDescriptorSet::setBuffer(const std::string& name, const std::shared_ptr<UniformBuffer>& buffer) -> void 
	{
	}

	auto VulkanDescriptorSet::getUnifromBuffer(const std::string& name) -> std::shared_ptr<UniformBuffer> 
	{
	}

	auto VulkanDescriptorSet::setUniform(const std::string& bufferName, const std::string& uniformName, void* data) -> void 
	{
	}

	auto VulkanDescriptorSet::setUniform(const std::string& bufferName, const std::string& uniformName, void* data, uint32_t size) -> void 
	{
	}

	auto VulkanDescriptorSet::setUniformBufferData(const std::string& bufferName, void* data) -> void 
	{
	}
};