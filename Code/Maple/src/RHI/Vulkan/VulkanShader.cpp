//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "VulkanShader.h"
#include "FileSystem/File.h"
#include "Others/Console.h"
#include "Others/StringUtils.h"
#include "VulkanCommandBuffer.h"
#include "VulkanDevice.h"
#include "VulkanPipeline.h"
#include <spirv_cross.hpp>

namespace maple
{
	namespace
	{
		inline auto getStrideFromVulkanFormat(VkFormat format) -> uint32_t
		{
			switch (format)
			{
			case VK_FORMAT_R8_SINT:
			case VK_FORMAT_R32_SINT:
				return sizeof(int);
			case VK_FORMAT_R32_SFLOAT:
				return sizeof(float);
			case VK_FORMAT_R32G32_SFLOAT:
				return sizeof(glm::vec2);
			case VK_FORMAT_R32G32B32_SFLOAT:
				return sizeof(glm::vec3);
			case VK_FORMAT_R32G32B32A32_SFLOAT:
				return sizeof(glm::vec4);
			case VK_FORMAT_R32G32_SINT:
				return sizeof(glm::ivec2);
			case VK_FORMAT_R32G32B32_SINT:
				return sizeof(glm::ivec3);
			case VK_FORMAT_R32G32B32A32_SINT:
				return sizeof(glm::ivec4);
			case VK_FORMAT_R32G32_UINT:
				return sizeof(glm::ivec2);
			case VK_FORMAT_R32G32B32_UINT:
				return sizeof(glm::ivec3);
			case VK_FORMAT_R32G32B32A32_UINT:
				return sizeof(glm::ivec4);
			default:
				LOGE("Unsupported Vulkan Format {0} , {1} : {2}", format, __FUNCTION__, __LINE__);
				return 0;
			}

			return 0;
		}

		inline auto getVulkanFormat(const spirv_cross::SPIRType type)
		{
			VkFormat uintTypes[] =
			{
				VK_FORMAT_R32_UINT,
				VK_FORMAT_R32G32_UINT,
				VK_FORMAT_R32G32B32_UINT,
				VK_FORMAT_R32G32B32A32_UINT };

			VkFormat intTypes[] =
			{
				VK_FORMAT_R32_SINT,
				VK_FORMAT_R32G32_SINT,
				VK_FORMAT_R32G32B32_SINT,
				VK_FORMAT_R32G32B32A32_SINT };

			VkFormat floatTypes[] =
			{
				VK_FORMAT_R32_SFLOAT,
				VK_FORMAT_R32G32_SFLOAT,
				VK_FORMAT_R32G32B32_SFLOAT,
				VK_FORMAT_R32G32B32A32_SFLOAT };

			VkFormat doubleTypes[] =
			{
				VK_FORMAT_R64_SFLOAT,
				VK_FORMAT_R64G64_SFLOAT,
				VK_FORMAT_R64G64B64_SFLOAT,
				VK_FORMAT_R64G64B64A64_SFLOAT,
			};

			switch (type.basetype)
			{
			case spirv_cross::SPIRType::UInt:
				return uintTypes[type.vecsize - 1];
			case spirv_cross::SPIRType::Int:
				return intTypes[type.vecsize - 1];
			case spirv_cross::SPIRType::Float:
				return floatTypes[type.vecsize - 1];
			case spirv_cross::SPIRType::Double:
				return doubleTypes[type.vecsize - 1];
			default:
				LOGC("Cannot find VK_Format : {0}", type.basetype);
				return VK_FORMAT_R32G32B32A32_SFLOAT;
			}
		}

		inline auto sprivTypeToDataType(const spirv_cross::SPIRType type)
		{
			switch (type.basetype)
			{
			case spirv_cross::SPIRType::Boolean:
				return ShaderDataType::Bool;
			case spirv_cross::SPIRType::Int:
				if (type.vecsize == 1)
					return ShaderDataType::Int;
				if (type.vecsize == 2)
					return ShaderDataType::IVec2;
				if (type.vecsize == 3)
					return ShaderDataType::IVec3;
				if (type.vecsize == 4)
					return ShaderDataType::IVec4;
			case spirv_cross::SPIRType::UInt:
				return ShaderDataType::UInt;
			case spirv_cross::SPIRType::Float:
				if (type.columns == 3)
					return ShaderDataType::Mat3;
				if (type.columns == 4)
					return ShaderDataType::Mat4;

				if (type.vecsize == 1)
					return ShaderDataType::Float32;
				if (type.vecsize == 2)
					return ShaderDataType::Vec2;
				if (type.vecsize == 3)
					return ShaderDataType::Vec3;
				if (type.vecsize == 4)
					return ShaderDataType::Vec4;
				break;
			case spirv_cross::SPIRType::Struct:
				return ShaderDataType::Struct;
			}
			LOGW("Unknown spirv type!");
			return ShaderDataType::None;
		}

		inline auto getImageFormatName(spv::ImageFormat format) -> const char*
		{
			switch (format)
			{
			case spv::ImageFormat::ImageFormatRgba32f:
				return "rgba32f";
			case spv::ImageFormat::ImageFormatRgba16f:
				return "rgba16f";
			case spv::ImageFormat::ImageFormatR32f:
				return "r32f";
			case spv::ImageFormat::ImageFormatRgba8:
				return "rgba8";
			case spv::ImageFormat::ImageFormatRgba8Snorm:
				return "rgba8_snorm";
			case spv::ImageFormat::ImageFormatRg32f:
				return "rg32f";
			case spv::ImageFormat::ImageFormatRg16f:
				return "rg16f";
			case spv::ImageFormat::ImageFormatRgba32i:
				return "rgba32i";
			case spv::ImageFormat::ImageFormatRgba16i:
				return "rgba16i";
			case spv::ImageFormat::ImageFormatR32i:
				return "r32i";
			case spv::ImageFormat::ImageFormatRgba8i:
				return "rgba8i";
			case spv::ImageFormat::ImageFormatRg32i:
				return "rg32i";
			case spv::ImageFormat::ImageFormatRg16i:
				return "rg16i";
			case spv::ImageFormat::ImageFormatRgba32ui:
				return "rgba32ui";
			case spv::ImageFormat::ImageFormatRgba16ui:
				return "rgba16ui";
			case spv::ImageFormat::ImageFormatR32ui:
				return "r32ui";
			case spv::ImageFormat::ImageFormatRgba8ui:
				return "rgba8ui";
			case spv::ImageFormat::ImageFormatRg32ui:
				return "rg32ui";
			case spv::ImageFormat::ImageFormatRg16ui:
				return "rg16ui";
			case spv::ImageFormat::ImageFormatR11fG11fB10f:
				return "r11f_g11f_b10f";
			case spv::ImageFormat::ImageFormatR16f:
				return "r16f";
			case spv::ImageFormat::ImageFormatRgb10A2:
				return "rgb10_a2";
			case spv::ImageFormat::ImageFormatR8:
				return "r8";
			case spv::ImageFormat::ImageFormatRg8:
				return "rg8";
			case spv::ImageFormat::ImageFormatR16:
				return "r16";
			case spv::ImageFormat::ImageFormatRg16:
				return "rg16";
			case spv::ImageFormat::ImageFormatRgba16:
				return "rgba16";
			case spv::ImageFormat::ImageFormatR16Snorm:
				return "r16_snorm";
			case spv::ImageFormat::ImageFormatRg16Snorm:
				return "rg16_snorm";
			case spv::ImageFormat::ImageFormatRgba16Snorm:
				return "rgba16_snorm";
			case spv::ImageFormat::ImageFormatR8Snorm:
				return "r8_snorm";
			case spv::ImageFormat::ImageFormatRg8Snorm:
				return "rg8_snorm";
			case spv::ImageFormat::ImageFormatR8ui:
				return "r8ui";
			case spv::ImageFormat::ImageFormatRg8ui:
				return "rg8ui";
			case spv::ImageFormat::ImageFormatR16ui:
				return "r16ui";
			case spv::ImageFormat::ImageFormatRgb10a2ui:
				return "rgb10_a2ui";
			case spv::ImageFormat::ImageFormatR8i:
				return "r8i";
			case spv::ImageFormat::ImageFormatRg8i:
				return "rg8i";
			case spv::ImageFormat::ImageFormatR16i:
				return "r16i";
			default:
			case spv::ImageFormat::ImageFormatUnknown:
				return nullptr;
			}
		}

	}        // namespace

	VulkanShader::VulkanShader(const std::string& path) :
		filePath(path)
	{
		name = StringUtils::getFileName(filePath);
		auto bytes = File::read(filePath);
		source = { bytes->begin(), bytes->end() };
		if (!source.empty())
		{
			init();
		}
	}

	VulkanShader::VulkanShader(const std::vector<uint32_t>& vertData, const std::vector<uint32_t>& fragData)
	{
		LOGW("{0} did not implement", __FUNCTION__);
	}

	VulkanShader::~VulkanShader()
	{
		unload();
	}

	auto VulkanShader::bindPushConstants(CommandBuffer* cmdBuffer, Pipeline* pipeline) -> void
	{
		uint32_t index = 0;
		for (auto& pc : pushConstants)
		{
			vkCmdPushConstants(
				static_cast<VulkanCommandBuffer*>(cmdBuffer)->getCommandBuffer(),
				static_cast<VulkanPipeline*>(pipeline)->getPipelineLayout(),
				VkConverter::shaderTypeToVK(pc.shaderStage)
				, index, pc.size, pc.data.data());
		}
	}

	auto VulkanShader::init() -> void
	{
		PROFILE_FUNCTION();
		uint32_t currentShaderStage = 0;

		std::vector<std::string> lines;
		StringUtils::split(source, "\n", lines);
		std::unordered_map<ShaderType, std::string> sources;
		parseSource(lines, sources);

		for (auto& source : sources)
		{
			shaderTypes.emplace_back(source.first);
		}

		shaderStages.resize(sources.size());

		LOGI("Loading Shader : {0}", name);

		for (auto& source : sources)
		{
			auto buffer = File::read(source.second);
			auto size = buffer->size() / sizeof(uint32_t);
			loadShader({ reinterpret_cast<uint32_t*>(buffer->data()), reinterpret_cast<uint32_t*>(buffer->data()) + size }, source.first, currentShaderStage);
			currentShaderStage++;
		}

		createPipelineLayout();
	}

	auto VulkanShader::createPipelineLayout() -> void
	{
		std::vector<std::vector<DescriptorLayoutInfo>> layouts;

		for (auto& descriptorLayout : descriptorLayoutInfo)
		{
			if (layouts.size() < descriptorLayout.setID + 1)
			{
				layouts.emplace_back();
			}
			layouts[descriptorLayout.setID].emplace_back(descriptorLayout);
		}

		for (auto& l : layouts)
		{
			std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;
			std::vector<VkDescriptorBindingFlags>     layoutBindingFlags;
			setLayoutBindings.reserve(l.size());
			layoutBindingFlags.reserve(l.size());

			for (uint32_t i = 0; i < l.size(); i++)
			{
				auto& info = l[i];

				VkDescriptorSetLayoutBinding setLayoutBinding{};
				setLayoutBinding.descriptorType = VkConverter::descriptorTypeToVK(info.type);
				setLayoutBinding.stageFlags = VkConverter::shaderTypeToVK(info.stage);
				setLayoutBinding.binding = info.binding;
				setLayoutBinding.descriptorCount = info.count;

				bool isArray = info.count > 1;
				layoutBindingFlags.emplace_back(isArray ? VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT : 0);
				setLayoutBindings.emplace_back(setLayoutBinding);
			}

			VkDescriptorSetLayoutBindingFlagsCreateInfoEXT flagsInfo = {};
			flagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
			flagsInfo.pNext = nullptr;
			flagsInfo.bindingCount = static_cast<uint32_t>(layoutBindingFlags.size());
			flagsInfo.pBindingFlags = layoutBindingFlags.data();

			// Pipeline layout
			VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
			setLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			setLayoutCreateInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
			setLayoutCreateInfo.pBindings = setLayoutBindings.data();
			setLayoutCreateInfo.pNext = &flagsInfo;
			VkDescriptorSetLayout layout;
			vkCreateDescriptorSetLayout(*VulkanDevice::get(), &setLayoutCreateInfo, VK_NULL_HANDLE, &layout);

			descriptorSetLayouts.emplace_back(layout);
		}

		std::vector<VkPushConstantRange> pushConstantRanges;

		for (auto& pushConst : pushConstants)
		{
			pushConstantRanges.push_back(VulkanHelper::pushConstantRange(VkConverter::shaderTypeToVK(pushConst.shaderStage), pushConst.size, pushConst.offset));
		}

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();

		pipelineLayoutCreateInfo.pushConstantRangeCount = uint32_t(pushConstantRanges.size());
		pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();

		VK_CHECK_RESULT(vkCreatePipelineLayout(*VulkanDevice::get(), &pipelineLayoutCreateInfo, VK_NULL_HANDLE, &pipelineLayout));
	}

	auto VulkanShader::unload() const -> void
	{
		PROFILE_FUNCTION();
		for (auto& stage : shaderStages)
		{
			vkDestroyShaderModule(*VulkanDevice::get(), stage.module, nullptr);
		}

		for (auto& descriptorLayout : descriptorSetLayouts)
			vkDestroyDescriptorSetLayout(*VulkanDevice::get(), descriptorLayout, VK_NULL_HANDLE);

		vkDestroyPipelineLayout(*VulkanDevice::get(), pipelineLayout, VK_NULL_HANDLE);
	}

	auto VulkanShader::loadShader(const std::vector<uint32_t>& spvCode, ShaderType shaderType, int32_t currentShaderStage) -> void
	{
		VkShaderModuleCreateInfo shaderCreateInfo{};
		shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderCreateInfo.codeSize = spvCode.size() * 4; // len is same as uint8_t 
		shaderCreateInfo.pCode = spvCode.data();
		shaderCreateInfo.pNext = VK_NULL_HANDLE;

		spirv_cross::Compiler        comp(spvCode.data(), spvCode.size());
		spirv_cross::ShaderResources resources = comp.get_shader_resources();

		if (shaderType == ShaderType::Vertex)
		{
			//Vertex Layout
			vertexInputStride = 0;

			auto copyInput = resources.stage_inputs;

			std::sort(copyInput.begin(), copyInput.end(), [&](const auto & left, const auto & right) {
				auto location = comp.get_decoration(left.id, spv::DecorationLocation);
				auto location1 = comp.get_decoration(right.id, spv::DecorationLocation);
				return location < location1;
			});

			for (const spirv_cross::Resource& resource : copyInput)
			{
				const spirv_cross::SPIRType& InputType = comp.get_type(resource.type_id);
				VkVertexInputAttributeDescription description = {};
				description.binding = comp.get_decoration(resource.id, spv::DecorationBinding);
				description.location = comp.get_decoration(resource.id, spv::DecorationLocation);
				LOGV("Load Vertex , binding : {} location : {}, name : {}", description.binding, description.location, name);
				description.offset = vertexInputStride;
				description.format = getVulkanFormat(InputType);
				vertexInputAttributeDescriptions.emplace_back(description);
				vertexInputStride += getStrideFromVulkanFormat(description.format);
			}
		}

		if (shaderType == ShaderType::Compute)
		{
			computeShader = true;
			localSizeX = comp.get_execution_mode_argument(spv::ExecutionMode::ExecutionModeLocalSize, 0);
			localSizeY = comp.get_execution_mode_argument(spv::ExecutionMode::ExecutionModeLocalSize, 1);
			localSizeZ = comp.get_execution_mode_argument(spv::ExecutionMode::ExecutionModeLocalSize, 2);
			//todo...reflect the image
		}


		for (auto& resource : resources.storage_images)
		{
			auto& glslType = comp.get_type(resource.base_type_id);
			auto& glslType2 = comp.get_type(resource.type_id);

			if (glslType.basetype == spirv_cross::SPIRType::Image)
			{
				const char* fmt = getImageFormatName(glslType.image.format);
				uint32_t    set = comp.get_decoration(resource.id, spv::DecorationDescriptorSet);
				uint32_t    binding = comp.get_decoration(resource.id, spv::DecorationBinding);

				LOGV("VK -> Load Image, type {0}, set {1}, binding {2}, name {3}", fmt, set, binding, resource.name);
				auto& type = comp.get_type(resource.type_id);
				descriptorLayoutInfo.push_back({ DescriptorType::Image, shaderType, binding, set, type.array.size() ? uint32_t(type.array[0]) : 1 });

				auto& descriptorInfo = descriptorInfos[set];
				auto& descriptor = descriptorInfo.emplace_back();

				descriptor.offset = 0;
				descriptor.size = 0;
				descriptor.binding = binding;
				descriptor.name = resource.name;
				descriptor.shaderType = shaderType;
				descriptor.type = DescriptorType::Image;
				//descriptor.accessFlag = glslType.image.access;
				descriptor.accessFlag = spv::AccessQualifierReadWrite;
				descriptor.format = spirvTypeToTextureType(glslType.image.format);
			}
		}


		//Descriptor Layout
		for (auto& u : resources.uniform_buffers)
		{
			uint32_t set = comp.get_decoration(u.id, spv::DecorationDescriptorSet);
			uint32_t binding = comp.get_decoration(u.id, spv::DecorationBinding);
			auto& type = comp.get_type(u.type_id);

			LOGI("Uniform {0} at set = {1}, binding = {2}", u.name, set, binding);
			descriptorLayoutInfo.push_back({ DescriptorType::UniformBuffer, shaderType, binding, set, type.array.size() ? uint32_t(type.array[0]) : 1 });

			auto& bufferType = comp.get_type(u.base_type_id);
			auto  bufferSize = comp.get_declared_struct_size(bufferType);
			auto  memberCount = (int32_t)bufferType.member_types.size();

			auto& descriptorInfo = descriptorInfos[set];
			auto& descriptor = descriptorInfo.emplace_back();
			descriptor.binding = binding;
			descriptor.size = (uint32_t)bufferSize;
			descriptor.name = u.name;
			descriptor.offset = 0;
			descriptor.shaderType = shaderType;
			descriptor.type = DescriptorType::UniformBuffer;
			descriptor.buffer = nullptr;

			for (int32_t i = 0; i < memberCount; i++)
			{
				auto        type = comp.get_type(bufferType.member_types[i]);
				const auto& memberName = comp.get_member_name(bufferType.self, i);
				auto        size = comp.get_declared_struct_member_size(bufferType, i);
				auto        offset = comp.type_struct_member_offset(bufferType, i);

				std::string uniformName = u.name + "." + memberName;

				auto& member = descriptor.members.emplace_back();
				member.name = memberName;
				member.offset = offset;
				member.size = (uint32_t)size;
				member.fullName = uniformName;
				LOGI("{0} - Size {1}, offset {2}", uniformName, size, offset);
			}
		}

		for (auto& u : resources.push_constant_buffers)
		{
			uint32_t set = comp.get_decoration(u.id, spv::DecorationDescriptorSet);
			uint32_t binding = comp.get_decoration(u.id, spv::DecorationBinding);
			uint32_t binding3 = comp.get_decoration(u.id, spv::DecorationOffset);

			auto& type = comp.get_type(u.type_id);

			auto ranges = comp.get_active_buffer_ranges(u.id);

			uint32_t size = 0;
			for (auto& range : ranges)
			{
				LOGI("\tAccessing Member {0} offset {1}, size {2}", range.index, range.offset, range.range);
				size += uint32_t(range.range);
			}

			LOGI("Push Constant {0} at set = {1}, binding = {2}", u.name.c_str(), set, binding);

			auto& push = pushConstants.emplace_back();
			push.size = size;
			push.shaderStage = shaderType;
			push.data.resize(size);
			push.name = u.name;

			auto& bufferType = comp.get_type(u.base_type_id);
			auto    bufferSize = comp.get_declared_struct_size(bufferType);
			int32_t memberCount = (int32_t)bufferType.member_types.size();

			for (int32_t i = 0; i < memberCount; i++)
			{
				auto        type = comp.get_type(bufferType.member_types[i]);
				const auto& memberName = comp.get_member_name(bufferType.self, i);
				auto        size = comp.get_declared_struct_member_size(bufferType, i);
				auto        offset = comp.type_struct_member_offset(bufferType, i);

				std::string uniformName = u.name + "." + memberName;

				auto& member = push.members.emplace_back();
				member.size = (uint32_t)size;
				member.offset = offset;
				member.type = sprivTypeToDataType(type);
				member.fullName = uniformName;
				member.name = memberName;
			}
		}

		for (auto& u : resources.sampled_images)
		{
			uint32_t set = comp.get_decoration(u.id, spv::DecorationDescriptorSet);
			uint32_t binding = comp.get_decoration(u.id, spv::DecorationBinding);

			auto& descriptorInfo = descriptorInfos[set];
			auto& descriptor = descriptorInfo.emplace_back();

			auto& type = comp.get_type(u.type_id);
			LOGI("Found Sampled Image {0} at set = {1}, binding = {2}", u.name.c_str(), set, binding);

			descriptorLayoutInfo.push_back({ DescriptorType::ImageSampler, shaderType, binding, set, type.array.size() ? uint32_t(type.array[0]) : 1 });
			descriptor.binding = binding;
			descriptor.name = u.name;
			descriptor.offset = 0;
			descriptor.size = 0;
			descriptor.shaderType = shaderType;
		}

		shaderStages[currentShaderStage].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[currentShaderStage].stage = VkConverter::shaderTypeToVK(shaderType);
		shaderStages[currentShaderStage].pName = "main";
		shaderStages[currentShaderStage].pNext = VK_NULL_HANDLE;

		VK_CHECK_RESULT(vkCreateShaderModule(*VulkanDevice::get(), &shaderCreateInfo, nullptr, &shaderStages[currentShaderStage].module));
	}

};        // namespace maple
