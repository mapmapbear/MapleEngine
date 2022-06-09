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
#include "VulkanCommandBuffer.h"

#include "Application.h"

namespace maple
{
	namespace
	{
		inline auto transitionImageLayout(const CommandBuffer* cmd, Texture* texture,bool sampler2d, VkFormat vkFormat, int32_t mipLevel)
		{
			if (!texture)
				return;

			const VulkanCommandBuffer *commandBuffer = (VulkanCommandBuffer *)cmd;
			if (texture->getType() == TextureType::Color)
			{
				if (sampler2d) 
				{
					if (((VulkanTexture2D*)texture)->getImageLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
					{
						((VulkanTexture2D*)texture)->transitionImage(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, commandBuffer);
					}
				}
				else 
				{
					((VulkanTexture2D*)texture)->transitionImage(VK_IMAGE_LAYOUT_GENERAL, commandBuffer);
				}
			}
			else if (texture->getType() == TextureType::Color3D)
			{
				((VulkanTexture3D*)texture)->transitionImage2( VK_IMAGE_LAYOUT_GENERAL, commandBuffer, mipLevel);
			}
			else if (texture->getType() == TextureType::Cube)
			{
				if (((VulkanTextureCube *) texture)->getImageLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
				{
					((VulkanTextureCube *) texture)->transitionImage(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, commandBuffer);
				}
			}
			else if (texture->getType() == TextureType::Depth)
			{
				((VulkanTextureDepth *) texture)->transitionImage(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, commandBuffer);
			}
			else if (texture->getType() == TextureType::DepthArray)
			{
				((VulkanTextureDepthArray *) texture)->transitionImage(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, commandBuffer);
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
		uniformBuffers.resize(framesInFlight);
		for (auto &descriptor : descriptors)
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

		descriptorSet.resize(framesInFlight, nullptr);
		for (uint32_t frame = 0; frame < framesInFlight; frame++)
		{
			descriptorDirty[frame] = true;
			VK_CHECK_RESULT(vkAllocateDescriptorSets(*VulkanDevice::get(), &descriptorSetAllocateInfo, &descriptorSet[frame]));
		}

		memset(imageInfoPool.data(), 0, sizeof(VkDescriptorImageInfo) * MAX_IMAGE_INFOS);
	}

	VulkanDescriptorSet::~VulkanDescriptorSet()
	{
		PROFILE_FUNCTION();
	}

	auto VulkanDescriptorSet::update(const CommandBuffer* commandBuffer) -> void
	{
		PROFILE_FUNCTION();

		dynamic = false;

		int32_t  descriptorWritesCount = 0;

		const auto vkCmd = static_cast<const VulkanCommandBuffer*>(commandBuffer);

		auto compute = vkCmd->getCommandBuffeType() == CommandBufferType::Compute;

		currentFrame = compute ? 0 : Application::getGraphicsContext()->getSwapChain()->getCurrentBufferIndex();

		for (auto &bufferInfo : uniformBuffersData)
		{
			if (bufferInfo.second.hasUpdated[currentFrame])
			{
				if (bufferInfo.second.dynamic)
					uniformBuffers[currentFrame][bufferInfo.first]->setDynamicData(bufferInfo.second.localStorage.size, 0, bufferInfo.second.localStorage.data);
				else
					uniformBuffers[currentFrame][bufferInfo.first]->setData(bufferInfo.second.localStorage.data);

				bufferInfo.second.hasUpdated[currentFrame] = false;
			}
		}

		if (descriptorDirty[currentFrame])
		{
			descriptorDirty[currentFrame] = false;
			int32_t imageIndex            = 0;
			int32_t index                 = 0;

			for (auto &imageInfo : descriptors)
			{
				if (imageInfo.type == DescriptorType::ImageSampler || imageInfo.type == DescriptorType::Image)
				{
					if (!imageInfo.textures.empty())
					{
						auto validCount = 0;
						for (uint32_t i = 0; i < imageInfo.textures.size(); i++)
						{
							if (imageInfo.textures[i])
							{
								transitionImageLayout(
									commandBuffer,imageInfo.textures[i].get(), 
									imageInfo.type == DescriptorType::ImageSampler,
									VkConverter::textureFormatToVK(imageInfo.format, false,true),
									imageInfo.mipmapLevel
								);

								const auto &des = *static_cast<VkDescriptorImageInfo *>(imageInfo.textures[i]->getDescriptorInfo(imageInfo.mipmapLevel));
								imageInfoPool[i + imageIndex] = des;
								validCount++;
							}
						}

						if (validCount > 0)
						{
							VkWriteDescriptorSet writeDescriptorSet{};
							writeDescriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
							writeDescriptorSet.dstSet          = descriptorSet[currentFrame];
							writeDescriptorSet.descriptorType  = VkConverter::descriptorTypeToVK(imageInfo.type);
							writeDescriptorSet.dstBinding      = imageInfo.binding;
							writeDescriptorSet.pImageInfo      = &imageInfoPool[imageIndex];
							writeDescriptorSet.descriptorCount = validCount;

							MAPLE_ASSERT(writeDescriptorSet.descriptorCount != 0, "writeDescriptorSet.descriptorCount should be greater than zero");

							writeDescriptorSetPool[descriptorWritesCount] = writeDescriptorSet;
							imageIndex++;
							descriptorWritesCount++;
						}
					}
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

			if (descriptorWritesCount > 0)
				vkUpdateDescriptorSets(*VulkanDevice::get(), descriptorWritesCount, writeDescriptorSetPool.data(), 0, nullptr);
		}
	}

	auto VulkanDescriptorSet::getDescriptorSet() -> VkDescriptorSet
	{
		return descriptorSet[currentFrame];
	}

	auto VulkanDescriptorSet::setTexture(const std::string &name, const std::vector<std::shared_ptr<Texture>> &textures, int32_t mipLevel) -> void
	{
		for (auto &descriptor : descriptors)
		{
			if ( (descriptor.type == DescriptorType::ImageSampler || descriptor.type == DescriptorType::Image)
				&& descriptor.name == name)
			{
				descriptor.textures = textures;
				descriptor.mipmapLevel = mipLevel;
				descriptorDirty[0]  = true;
				descriptorDirty[1]  = true;
				descriptorDirty[2]  = true;
			}
		}
	}

	auto VulkanDescriptorSet::setTexture(const std::string &name, const std::shared_ptr<Texture> &texture, int32_t mipLevel) -> void
	{
		PROFILE_FUNCTION();
		setTexture(name, std::vector<std::shared_ptr<Texture>>{texture},mipLevel);
	}

	auto VulkanDescriptorSet::setBuffer(const std::string &name, const std::shared_ptr<UniformBuffer> &buffer) -> void
	{
	}

	auto VulkanDescriptorSet::getUnifromBuffer(const std::string &name) -> std::shared_ptr<UniformBuffer>
	{
		return nullptr;
	}

	auto VulkanDescriptorSet::setUniform(const std::string &bufferName, const std::string &uniformName, const void *data, bool dynamic) -> void
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
					iter->second.dynamic       = dynamic;
					return;
				}
			}
		}
		LOGW("Uniform not found {0}.{1}", bufferName, uniformName);
	}

	auto VulkanDescriptorSet::setUniform(const std::string &bufferName, const std::string &uniformName, const void *data, uint32_t size, bool dynamic) -> void
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
					iter->second.dynamic       = dynamic;
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
