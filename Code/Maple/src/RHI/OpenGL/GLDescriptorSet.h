#pragma once

#include "Engine/Buffer.h"
#include "RHI/DescriptorSet.h"
#include <memory>
#include <vector>

namespace maple
{
	class GLShader;

	class GLDescriptorSet : public DescriptorSet
	{
	  public:
		GLDescriptorSet(const DescriptorInfo &descriptorDesc);

		~GLDescriptorSet(){};

		auto update() -> void override;
		auto setTexture(const std::string &name, const std::vector<std::shared_ptr<Texture>> &textures) -> void override;
		auto setTexture(const std::string &name, const std::shared_ptr<Texture> &textures) -> void override;
		auto setBuffer(const std::string &name, const std::shared_ptr<UniformBuffer> &buffer) -> void override;
		auto setUniform(const std::string &bufferName, const std::string &uniformName, const void *data) -> void override;
		auto setUniform(const std::string &bufferName, const std::string &uniformName, const void *data, uint32_t size) -> void override;
		auto setUniformBufferData(const std::string &bufferName, const void *data) -> void override;
		auto getUnifromBuffer(const std::string &name) -> std::shared_ptr<UniformBuffer> override;
		auto bind(uint32_t offset = 0) -> void;

		inline auto setDynamicOffset(uint32_t offset) -> void override
		{
			dynamicOffset = offset;
		}
		inline auto getDynamicOffset() const -> uint32_t override
		{
			return dynamicOffset;
		}

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
		std::unordered_map<std::string, UniformBufferInfo> uniformBuffers;
	};
}        // namespace maple
