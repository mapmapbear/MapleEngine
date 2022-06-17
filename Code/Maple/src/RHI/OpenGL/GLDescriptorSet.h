//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Engine/Buffer.h"
#include "RHI/DescriptorSet.h"
#include <memory>
#include <vector>

namespace maple
{
	class GLShader;
	class StorageBuffer;

	class GLDescriptorSet : public DescriptorSet
	{
	  public:
		GLDescriptorSet(const DescriptorInfo &descriptorDesc);

		~GLDescriptorSet(){};

		auto update(const CommandBuffer* cmd) -> void override;
		auto setTexture(const std::string &name, const std::vector<std::shared_ptr<Texture>> &textures, int32_t mipLevel = -1) -> void override;
		auto setTexture(const std::string &name, const std::shared_ptr<Texture> &textures, int32_t mipLevel = -1) -> void override;
		auto setBuffer(const std::string &name, const std::shared_ptr<UniformBuffer> &buffer) -> void override;
		auto setUniform(const std::string &bufferName, const std::string &uniformName, const void *data, bool dynamic) -> void override;
		auto setUniform(const std::string &bufferName, const std::string &uniformName, const void *data, uint32_t size, bool dynamic) -> void override;
		auto setUniformBufferData(const std::string &bufferName, const void *data) -> void override;
		auto getUnifromBuffer(const std::string &name) -> std::shared_ptr<UniformBuffer> override;
		auto bind(uint32_t offset = 0) -> void;
		auto setSSBO(const std::string& name, uint32_t size, const void* data) -> void override;

		inline auto setDynamicOffset(uint32_t offset) -> void override
		{
			dynamicOffset = offset;
		}
		inline auto getDynamicOffset() const -> uint32_t override
		{
			return dynamicOffset;
		}

		auto getDescriptors() const -> const std::vector<Descriptor> & override { return descriptors; }

		auto toIntID() const -> const uint64_t override { return -1; };

	  private:
		uint32_t                dynamicOffset = 0;
		GLShader *              shader        = nullptr;
		std::vector<Descriptor> descriptors;

		struct UniformBufferInfo
		{
			std::shared_ptr<UniformBuffer> uniformBuffer;
			std::vector<BufferMemberInfo>  members;
			Buffer                         localStorage;
			bool                           dirty;
		};

		struct SSBOInfo 
		{
			std::shared_ptr<StorageBuffer> ssbo;
			Buffer                         localStorage;
			bool                           dirty;
		};

		std::unordered_map<std::string, UniformBufferInfo> uniformBuffers;
		std::unordered_map<std::string, SSBOInfo> ssboBuffers;
	};
}        // namespace maple
