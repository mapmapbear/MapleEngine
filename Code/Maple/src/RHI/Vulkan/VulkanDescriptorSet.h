//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "VulkanHelper.h"
#include "Engine/Renderer/RenderParam.h"
#include "Engine/Interface/DescriptorSet.h"

namespace maple
{

	class VulkanShader;

	class VulkanDescriptorSet final : public DescriptorSet
	{
	public:
		VulkanDescriptorSet(const DescriptorCreateInfo & info);
		virtual auto update() -> void override;
		virtual auto setDynamicOffset(uint32_t offset) -> void override;
		virtual auto getDynamicOffset() const->uint32_t override;
		virtual auto setTexture(const std::string& name, const std::vector<std::shared_ptr<Texture>>& textures, TextureType textureType = TextureType(0)) -> void override;
		virtual auto setBuffer(const std::string& name, const std::shared_ptr<UniformBuffer>& buffer) -> void override;
		virtual auto getUnifromBuffer(const std::string& name)->std::shared_ptr<UniformBuffer> override;
		virtual auto setUniform(const std::string& bufferName, const std::string& uniformName, void* data) -> void override;
		virtual auto setUniform(const std::string& bufferName, const std::string& uniformName, void* data, uint32_t size) -> void override;
		virtual auto setUniformBufferData(const std::string& bufferName, void* data) -> void override;
	private:
		std::shared_ptr<Shader> shader;

	};
};