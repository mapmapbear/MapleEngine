//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "VulkanDescriptorSet.h"
#include "Engine/Profiler.h"
#include "Others/Console.h"
#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include "VulkanHelper.h"
#include "VulkanPipeline.h"
#include "VulkanRenderDevice.h"
#include "VulkanShader.h"
#include "VulkanTexture.h"
#include "VulkanUniformBuffer.h"

#include "Application.h"

namespace maple
{
	namespace
	{
		inline auto transitionImageLayout(Texture *texture)
		{
			if (!texture)
				return;

			auto commandBuffer = Application::getGraphicsContext()->getSwapChain()->getCurrentCommandBuffer();
			if (texture->getType() == TextureType::Color)
			{
				if (((VulkanTexture2D *) texture)->getImageLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
				{
					((VulkanTexture2D *) texture)->transitionImage(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, (VulkanCommandBuffer *) commandBuffer);
				}
			}
			else if (texture->getType() == TextureType::Depth)
			{
				((VulkanTextureDepth *) texture)->transitionImage(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, (VulkanCommandBuffer *) commandBuffer);
			}
			else if (texture->getType() == TextureType::DepthArray)
			{
				((VulkanTextureDepthArray *) texture)->transitionImage(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, (VulkanCommandBuffer *) commandBuffer);
			}
		}
	}        // namespace

	VulkanDescriptorSet::VulkanDescriptorSet(const DescriptorInfo &info)
	{
		PROFILE_FUNCTION();
		framesInFlight = uint32_t(Application::getGraphicsContext()->getSwapChain()->getSwapChainBufferCount());

		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
		descriptorSetAllocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.descriptorPool     = std::static_pointer_cast<VulkanRenderDevice>(Application::getRenderDevice())->getDescriptorPool();
		descriptorSetAllocateInfo.pSetLayouts        = static_cast<VulkanShader *>(info.shader)->getDescriptorLayout(info.layoutIndex);
		descriptorSetAllocateInfo.descriptorSetCount = info.count;
		descriptorSetAllocateInfo.pNext              = nullptr;

		shader      = info.shader;
		descriptors = shader->getDescriptorInfo(info.layoutIndex);

		for (auto &descriptor : descriptors.descriptors)
		{
			if (descriptor.type == DescriptorType::UniformBuffer)
			{
				for (uint32_t frame = 0; frame < framesInFlight; frame++)
				{
					auto buffer = UniformBuffer::create();
					buffer->init(descriptor.size, nullptr);
					uniformBuffers[frame][descriptor.name] = buffer;
				}

				Buffer localStorage;
				localStorage.allocate(descriptor.size);
				localStorage.initializeEmpty();

				UniformBufferInfo info;
				info.localStorage  = localStorage;
				info.hasUpdated[0] = false;
				info.hasUpdated[1] = false;
				info.hasUpdated[2] = false;

				info.members                        = descriptor.members;
				uniformBuffersData[descriptor.name] = info;
			}
		}

		for (uint32_t frame = 0; frame < framesInFlight; frame++)
		{
			descriptorDirty[frame] = true;
			descriptorSet[frame]   = nullptr;
			VK_CHECK_RESULT(vkAllocateDescriptorSets(*VulkanDevice::get(), &descriptorSetAllocateInfo, &descriptorSet[frame]));
		}
	}

	VulkanDescriptorSet::~VulkanDescriptorSet()
	{
	}

	auto VulkanDescriptorSet::update() -> void
	{
		PROFILE_FUNCTION();
		dynamic = false;

		int32_t  descriptorWritesCount = 0;
		uint32_t currentFrame          = Application::getGraphicsContext()->getSwapChain()->getCurrentBufferIndex();

		for (auto &bufferInfo : uniformBuffersData)
		{
			if (bufferInfo.second.hasUpdated[currentFrame])
			{
				uniformBuffers[currentFrame][bufferInfo.first]->setData(bufferInfo.second.localStorage.data);
				bufferInfo.second.hasUpdated[currentFrame] = false;
			}
		}

		if (descriptorDirty[currentFrame])
		{
			descriptorDirty[currentFrame] = false;
			int32_t imageIndex            = 0;
			int32_t index                 = 0;

			for (auto &imageInfo : descriptors.descriptors)
			{
				if (imageInfo.type == DescriptorType::ImageSampler)
				{
					for (uint32_t i = 0; i < imageInfo.textures.size(); i++)
					{
						transitionImageLayout(imageInfo.textures[i].get());

						VkDescriptorImageInfo &des                = *static_cast<VkDescriptorImageInfo *>(imageInfo.textures[i]->getDescriptorInfo());
						imageInfoPool[i + imageIndex].imageLayout = des.imageLayout;
						imageInfoPool[i + imageIndex].imageView   = des.imageView;
						imageInfoPool[i + imageIndex].sampler     = des.sampler;
					}

					VkWriteDescriptorSet writeDescriptorSet{};
					writeDescriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					writeDescriptorSet.dstSet          = descriptorSet[currentFrame];
					writeDescriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					writeDescriptorSet.dstBinding      = imageInfo.binding;
					writeDescriptorSet.pImageInfo      = &imageInfoPool[imageIndex];
					writeDescriptorSet.descriptorCount = imageInfo.textures.size();

					writeDescriptorSetPool[descriptorWritesCount] = writeDescriptorSet;
					imageIndex++;
					descriptorWritesCount++;
				}
				else if (imageInfo.type == DescriptorType::UniformBuffer)
				{
					auto buffer = std::static_pointer_cast<VulkanUniformBuffer>(uniformBuffers[currentFrame][imageInfo.name]);

					bufferInfoPool[index].buffer = buffer->getVkBuffer();
					bufferInfoPool[index].offset = imageInfo.offset;
					bufferInfoPool[index].range  = imageInfo.size;

					VkWriteDescriptorSet writeDescriptorSet{};
					writeDescriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					writeDescriptorSet.dstSet          = descriptorSet[currentFrame];
					writeDescriptorSet.descriptorType  = VkConverter::descriptorTypeToVK(imageInfo.type);
					writeDescriptorSet.dstBinding      = imageInfo.binding;
					writeDescriptorSet.pBufferInfo     = &bufferInfoPool[index];
					writeDescriptorSet.descriptorCount = 1;

					writeDescriptorSetPool[descriptorWritesCount] = writeDescriptorSet;
					index++;
					descriptorWritesCount++;

					if (imageInfo.type == DescriptorType::UniformBufferDynamic)
						dynamic = true;
				}
			}
			vkUpdateDescriptorSets(*VulkanDevice::get(), descriptorWritesCount, writeDescriptorSetPool.data(), 0, nullptr);
		}
	}

	auto VulkanDescriptorSet::getDescriptorSet() -> VkDescriptorSet
	{
		auto index = Application::getGraphicsContext()->getSwapChain()->getCurrentBufferIndex();
		return descriptorSet[index];
	}

	auto VulkanDescriptorSet::setTexture(const std::string &name, const std::vector<std::shared_ptr<Texture>> &textures) -> void
	{
		for (auto &descriptor : descriptors.descriptors)
		{
			if (descriptor.type == DescriptorType::ImageSampler && descriptor.name == name)
			{
				descriptor.textures = textures;
				descriptorDirty[0]  = true;
				descriptorDirty[1]  = true;
				descriptorDirty[2]  = true;
			}
		}
	}

	auto VulkanDescriptorSet::setTexture(const std::string &name, const std::shared_ptr<Texture> &texture) -> void
	{
		PROFILE_FUNCTION();
		setTexture(name, std::vector<std::shared_ptr<Texture>>{texture});
	}

	auto VulkanDescriptorSet::setBuffer(const std::string &name, const std::shared_ptr<UniformBuffer> &buffer) -> void
	{
	}

	auto VulkanDescriptorSet::getUnifromBuffer(const std::string &name) -> std::shared_ptr<UniformBuffer>
	{
		return nullptr;
	}

	auto VulkanDescriptorSet::setUniform(const std::string &bufferName, const std::string &uniformName, const void *data) -> void
	{
		PROFILE_FUNCTION();
		if (auto iter = uniformBuffersData.find(bufferName); iter != uniformBuffersData.end())
		{
			for (auto &member : iter->second.members)
			{
				if (member.name == uniformName)
				{
					iter->second.localStorage.write(data, member.size, member.offset);
					iter->second.hasUpdated[0] = true;
					iter->second.hasUpdated[1] = true;
					iter->second.hasUpdated[2] = true;
					return;
				}
			}
		}
		LOGW("Uniform not found {0}.{1}", bufferName, uniformName);
	}

	auto VulkanDescriptorSet::setUniform(const std::string &bufferName, const std::string &uniformName, const void *data, uint32_t size) -> void
	{
		PROFILE_FUNCTION();
		if (auto iter = uniformBuffersData.find(bufferName); iter != uniformBuffersData.end())
		{
			for (auto &member : iter->second.members)
			{
				if (member.name == uniformName)
				{
					iter->second.localStorage.write(data, size, member.offset);
					iter->second.hasUpdated[0] = true;
					iter->second.hasUpdated[1] = true;
					iter->second.hasUpdated[2] = true;
					return;
				}
			}
		}
		LOGW("Uniform not found {0}.{1}", bufferName, uniformName);
	}

	auto VulkanDescriptorSet::setUniformBufferData(const std::string &bufferName, const void *data) -> void
	{
		PROFILE_FUNCTION();
		if (auto iter = uniformBuffersData.find(bufferName); iter != uniformBuffersData.end())
		{
			iter->second.localStorage.write(data, iter->second.localStorage.getSize(), 0);
			iter->second.hasUpdated[0] = true;
			iter->second.hasUpdated[1] = true;
			iter->second.hasUpdated[2] = true;
			return;
		}
		LOGW("Uniform not found {0}.{1}", bufferName);
	}

};        // namespace maple
