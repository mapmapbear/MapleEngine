//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "GLDescriptorSet.h"
#include "Engine/Core.h"
#include "Engine/Profiler.h"
#include "GL.h"
#include "GLUniformBuffer.h"
#include "RHI/OpenGL/GLShader.h"
#include "RHI/Texture.h"
#include "RHI/UniformBuffer.h"

namespace maple
{
	GLDescriptorSet::GLDescriptorSet(const DescriptorInfo &descriptorDesc)
	{
		shader      = static_cast<GLShader *>(descriptorDesc.shader);
		descriptors = shader->getDescriptorInfo(descriptorDesc.layoutIndex);

		for (auto &descriptor : descriptors)
		{
			if (descriptor.type == DescriptorType::UniformBuffer)
			{
				auto buffer = UniformBuffer::create();
				buffer->init(descriptor.size, nullptr);
				descriptor.buffer = buffer;

				Buffer localStorage;
				localStorage.allocate(descriptor.size);
				localStorage.initializeEmpty();

				UniformBufferInfo info;
				info.uniformBuffer = buffer;
				info.localStorage  = localStorage;
				info.dirty         = false;
				info.members       = descriptor.members;
				uniformBuffers.emplace(descriptor.name, info);
			}
		}
	}

	auto GLDescriptorSet::update(const CommandBuffer* cmd, bool compute) -> void
	{
		PROFILE_FUNCTION();

		for (auto &bufferInfo : uniformBuffers)
		{
			if (bufferInfo.second.dirty)
			{
				bufferInfo.second.uniformBuffer->setData(bufferInfo.second.localStorage.data);
				bufferInfo.second.dirty = false;
			}
		}
	}

	auto GLDescriptorSet::setTexture(const std::string &name, const std::vector<std::shared_ptr<Texture>> &textures, uint32_t mipLevel) -> void
	{
		PROFILE_FUNCTION();
		for (auto &descriptor : descriptors)
		{
			if ((descriptor.type == DescriptorType::ImageSampler ||
			     descriptor.type == DescriptorType::Image) &&
			    descriptor.name == name)
			{
				descriptor.textures = textures;
				descriptor.mipmapLevel = mipLevel;
				return;
			}
		}
		LOGW("Texture not found {0}", name);
	}

	auto GLDescriptorSet::setTexture(const std::string &name, const std::shared_ptr<Texture> &texture, uint32_t mipLevel) -> void
	{
		setTexture(name, std::vector<std::shared_ptr<Texture>>{texture},mipLevel);
	}

	auto GLDescriptorSet::setBuffer(const std::string &name, const std::shared_ptr<UniformBuffer> &buffer) -> void
	{
		PROFILE_FUNCTION();
		for (auto &descriptor : descriptors)
		{
			if (descriptor.type == DescriptorType::UniformBuffer && descriptor.name == name)
			{
				descriptor.buffer = buffer;
				return;
			}
		}

		LOGW("Buffer not found {0}", name);
	}

	auto GLDescriptorSet::setUniform(const std::string &bufferName, const std::string &uniformName, const void *data, bool dynamic) -> void
	{
		PROFILE_FUNCTION();

		if (auto iter = uniformBuffers.find(bufferName); iter != uniformBuffers.end())
		{
			for (auto &member : iter->second.members)
			{
				if (member.name == uniformName)
				{
					iter->second.localStorage.write(data, member.size, member.offset);
					iter->second.dirty = true;
					return;
				}
			}
		}

		LOGW("Uniform not found {0}.{1}", bufferName, uniformName);
	}

	auto GLDescriptorSet::setUniform(const std::string &bufferName, const std::string &uniformName, const void *data, uint32_t size, bool dynamic) -> void
	{
		PROFILE_FUNCTION();

		if (auto iter = uniformBuffers.find(bufferName); iter != uniformBuffers.end())
		{
			for (auto &member : iter->second.members)
			{
				if (member.name == uniformName)
				{
					iter->second.localStorage.write(data, size, member.offset);
					iter->second.dirty = true;
					return;
				}
			}
		}
		LOGW("Uniform not found {0}.{1}", bufferName, uniformName);
	}

	auto GLDescriptorSet::setUniformBufferData(const std::string &bufferName, const void *data) -> void
	{
		PROFILE_FUNCTION();

		if (auto iter = uniformBuffers.find(bufferName); iter != uniformBuffers.end())
		{
			iter->second.localStorage.write(data, iter->second.localStorage.getSize(), 0);
			iter->second.dirty = true;
			return;
		}

		LOGW("Uniform not found {0}.", bufferName);
	}

	auto GLDescriptorSet::getUnifromBuffer(const std::string &name) -> std::shared_ptr<UniformBuffer>
	{
		PROFILE_FUNCTION();
		for (auto &descriptor : descriptors)
		{
			if (descriptor.type == DescriptorType::UniformBuffer && descriptor.name == name)
			{
				return descriptor.buffer;
			}
		}

		LOGW("Buffer not found {0}", name);
		return nullptr;
	}

	auto GLDescriptorSet::bind(uint32_t offset) -> void
	{
		PROFILE_FUNCTION();
		shader->bind();

		for (auto &descriptor : descriptors)
		{
			if (descriptor.type == DescriptorType::ImageSampler || descriptor.type == DescriptorType::Image)
			{
				if (descriptor.textures.size() == 1)
				{
					if (descriptor.textures[0])
					{
						if (descriptor.type == DescriptorType::ImageSampler)
						{
							descriptor.textures[0]->bind(descriptor.binding);
						}
						else
						{
							auto read  = descriptor.accessFlag == 0 || descriptor.accessFlag == 2;
							auto write = descriptor.accessFlag == 1 || descriptor.accessFlag == 2;
							descriptor.textures[0]->bindImageTexture(descriptor.binding, read, write, descriptor.mipmapLevel, 0);
						}
						shader->setUniform1i(descriptor.name, descriptor.binding);
					}
				}
				else
				{
					static const int32_t MAX_TEXTURE_UNITS           = 32;
					int32_t              samplers[MAX_TEXTURE_UNITS] = {0};

					MAPLE_ASSERT(MAX_TEXTURE_UNITS >= descriptor.textures.size(), "Texture Count greater than max");

					for (int32_t i = 0; i < descriptor.textures.size(); i++)
					{
						if (descriptor.textures[i])
						{
							if (descriptor.type == DescriptorType::ImageSampler)
							{
								descriptor.textures[i]->bind(descriptor.binding + i);
								
							}
							else 
							{
								auto read = descriptor.accessFlag == 0 || descriptor.accessFlag == 2;
								auto write = descriptor.accessFlag == 1 || descriptor.accessFlag == 2;
								descriptor.textures[i]->bindImageTexture(descriptor.binding + i, read, write, descriptor.mipmapLevel, 0);
							}
							samplers[i] = descriptor.binding + i;
						}
					}
					shader->setUniform1iv(descriptor.name, samplers, descriptor.textures.size());
				}
			}
			else
			{
				auto buffer = std::static_pointer_cast<GLUniformBuffer>(descriptor.buffer);

				if (!buffer)
					break;

				uint8_t *data = nullptr;
				uint32_t size = 0;

				if (buffer->isDynamic())
				{
					data = reinterpret_cast<uint8_t *>(buffer->getBuffer()) + offset;
					size = buffer->getTypeSize();
				}
				else
				{
					data = buffer->getBuffer();
					size = buffer->getSize();
				}

				{
					auto bufferHandle = buffer->getHandle();
					auto slot         = descriptor.binding;
					{
						PROFILE_SCOPE("glBindBufferBase");
						GLCall(glBindBufferBase(GL_UNIFORM_BUFFER, slot, bufferHandle));
					}

					if (buffer->isDynamic())
					{
						PROFILE_SCOPE("glBindBufferRange");
						GLCall(glBindBufferRange(GL_UNIFORM_BUFFER, slot, bufferHandle, offset, size));
					}

					if (descriptor.name != "")
					{
						PROFILE_SCOPE("glUniformBlockBinding");
						auto handle = shader->getProgramId();
						GLCall(auto loc = glGetUniformBlockIndex(handle, descriptor.name.c_str()));
						if (loc == GL_INVALID_INDEX)
						{
							LOGW("GLDescriptorSet {0}, GL_INVALID_INDEX , name : {1}", __LINE__, descriptor.name);
						}
						else
						{
							GLCall(glUniformBlockBinding(handle, loc, slot));
						}
					}
				}
			}
		}
	}
};        // namespace maple
