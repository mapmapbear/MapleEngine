//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include <memory>
#include <string>
#include <vector>

namespace maple
{
	class Pipeline;
	class Shader;
	class Texture;
	class UniformBuffer;
	class StorageBuffer;
	class CommandBuffer;
	class DescriptorPool;

	enum class TextureType : int32_t;
	enum class ShaderType : int32_t;
	enum class TextureFormat : int32_t;

	enum class DescriptorType
	{
		UniformBuffer,
		UniformBufferDynamic,
		ImageSampler,
		Image,
		Buffer,
		BufferDynamic,
		AccelerationStructure
	};

	enum class Format
	{
		R32G32B32A32_UINT,
		R32G32B32_UINT,
		R32G32_UINT,
		R32_UINT,
		R8_UINT,
		R32G32B32A32_INT,
		R32G32B32_INT,
		R32G32_INT,
		R32_INT,
		R32G32B32A32_FLOAT,
		R32G32B32_FLOAT,
		R32G32_FLOAT,
		R32_FLOAT
	};

	enum class ShaderDataType
	{
		None,
		Float32,
		Vec2,
		Vec3,
		Vec4,
		IVec2,
		IVec3,
		IVec4,
		Mat3,
		Mat4,
		Int32,
		Int,
		UInt,
		Bool,
		Struct,
		Mat4Array
	};

	struct BufferMemberInfo
	{
		uint32_t       size;
		uint32_t       offset;
		ShaderDataType type;
		std::string    name;
		std::string    fullName;
	};

	struct VertexInputDescription
	{
		uint32_t binding;
		uint32_t location;
		Format   format;
		uint32_t offset;
	};

	struct DescriptorPoolInfo
	{
		DescriptorType type;
		uint32_t       size;
	};

	struct DescriptorLayoutInfo
	{
		DescriptorType type;
		ShaderType     stage;
		uint32_t       binding       = 0;
		uint32_t       setID         = 0;
		uint32_t       count         = 1;
		bool           variableSized = false;
	};

	struct DescriptorLayout
	{
		uint32_t              count;
		DescriptorLayoutInfo *layoutInfo;
	};

	struct DescriptorInfo
	{
		uint32_t        layoutIndex;
		Shader *        shader;
		uint32_t        count         = 1;        //used in vulkan
		DescriptorPool *pool          = nullptr;
		uint32_t        variableCount = 0;        // designed for bindless in vulkan
	};

	struct Descriptor
	{
		std::vector<std::shared_ptr<Texture>> textures;
		std::shared_ptr<UniformBuffer>        buffer;
		std::shared_ptr<StorageBuffer>        ssbo;

		uint32_t    offset;
		uint32_t    size;
		uint32_t    binding;
		std::string name;

		DescriptorType type = DescriptorType::ImageSampler;
		ShaderType     shaderType;

		TextureFormat format;
		uint32_t      accessFlag  = 0;
		int32_t       mipmapLevel = -1;

		std::vector<BufferMemberInfo> members;
	};

	class DescriptorSet
	{
	  public:
		using Ptr = std::shared_ptr<DescriptorSet>;

		virtual ~DescriptorSet() = default;
		static auto create(const DescriptorInfo &desc) -> std::shared_ptr<DescriptorSet>;

		virtual auto update(const CommandBuffer *commandBuffer) -> void                                                                                       = 0;
		virtual auto setDynamicOffset(uint32_t offset) -> void                                                                                                = 0;
		virtual auto getDynamicOffset() const -> uint32_t                                                                                                     = 0;
		virtual auto setTexture(const std::string &name, const std::vector<std::shared_ptr<Texture>> &textures, int32_t mipLevel = -1) -> void                = 0;
		virtual auto setTexture(const std::string &name, const std::shared_ptr<Texture> &textures, int32_t mipLevel = -1) -> void                             = 0;
		virtual auto setBuffer(const std::string &name, const std::shared_ptr<UniformBuffer> &buffer) -> void                                                 = 0;
		virtual auto getUnifromBuffer(const std::string &name) -> std::shared_ptr<UniformBuffer>                                                              = 0;
		virtual auto setUniform(const std::string &bufferName, const std::string &uniformName, const void *data, bool dynamic = false) -> void                = 0;
		virtual auto setUniform(const std::string &bufferName, const std::string &uniformName, const void *data, uint32_t size, bool dynamic = false) -> void = 0;
		virtual auto setUniformBufferData(const std::string &bufferName, const void *data) -> void                                                            = 0;
		virtual auto setSSBO(const std::string &name, uint32_t size, const void *data) -> void                                                                = 0;
		virtual auto getDescriptors() const -> const std::vector<Descriptor> &                                                                                = 0;
		virtual auto toIntID() const -> const uint64_t                                                                                                        = 0;

		static auto canUpdate() -> bool;
		static auto toggleUpdate(bool update) -> void;
	};
}        // namespace maple
