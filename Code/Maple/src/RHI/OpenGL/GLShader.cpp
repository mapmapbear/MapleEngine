#include "GLShader.h"
#include "Engine/Profiler.h"
#include "FileSystem/File.h"
#include "Others/Console.h"
#include "Others/HashCode.h"
#include "Others/StringUtils.h"

#include "GLFunc.inl"

#include "Others/Console.h"

namespace maple
{
	namespace
	{
		const static Shader *shaderIndicator = nullptr;

		inline auto initBufferLayout(const spirv_cross::SPIRType type, BufferLayout &layout, const std::string &name, uint32_t location) -> void
		{
			switch (type.basetype)
			{
				case spirv_cross::SPIRType::Float:
					switch (type.vecsize)
					{
						case 1:
							layout.push<float>(name, location);
							break;
						case 2:
							layout.push<glm::vec2>(name, location);
							break;
						case 3:
							layout.push<glm::vec3>(name, location);
							break;
						case 4:
							layout.push<glm::vec4>(name, location);
							break;
					}
					break;
				case spirv_cross::SPIRType::Int:
					switch (type.vecsize)
					{
					case 1:
						layout.push<int32_t>(name, location);
						break;
					case 2:
						layout.push<glm::ivec2>(name, location);
						break;
					case 3:
						layout.push<glm::ivec3>(name, location);
						break;
					case 4:
						layout.push<glm::ivec4>(name, location);
						break;
					}
					break;
				case spirv_cross::SPIRType::Double:
					break;
				default:
					LOGE("Unknown spirv_cross::SPIRType %d ", type.basetype);
					break;
			}
		}

		inline auto getStrideFromOpenGLFormat(uint32_t format)
		{
			return 0;
		}

		inline auto typeToGL(ShaderType type)
		{
			switch (type)
			{
				case ShaderType::Vertex:
					return GL_VERTEX_SHADER;
				case ShaderType::Fragment:
					return GL_FRAGMENT_SHADER;
#ifndef PLATFORM_MOBILE
				case ShaderType::Geometry:
					return GL_GEOMETRY_SHADER;
				case ShaderType::TessellationControl:
					return GL_TESS_CONTROL_SHADER;
				case ShaderType::TessellationEvaluation:
					return GL_TESS_EVALUATION_SHADER;
				case ShaderType::Compute:
					return GL_COMPUTE_SHADER;
#endif
				default:
					LOGE("Unsupported Shader {0}", shaderTypeToName(type));
					return GL_VERTEX_SHADER;
			}
		}

	}        // namespace

	enum root_signature_spaces
	{
		PUSH_CONSTANT_REGISTER_SPACE = 0,
		DYNAMIC_OFFSET_SPACE,
		DESCRIPTOR_TABLE_INITIAL_SPACE,
	};

	GLShader::GLShader(const std::string &filePath, bool loadSPV) :
	    loadSPV(loadSPV), name(StringUtils::getFileName(filePath)), filePath(filePath)
	{
		reload();
	}

	auto GLShader::reload() -> void
	{
		descriptorInfos.clear();
		uniformLocations.clear();
		sampledLocations.clear();
		sources.clear();
		shaderCompilers.clear();
		pushConstants.clear();
		layout.reset();

		if (handle != -1)
		{
			GLCall(glDeleteProgram(handle));
			handle = -1;
		}
		auto bytes = File::read(filePath);
		source     = {bytes->begin(), bytes->end()};
		init();
	}

	GLShader::GLShader(const std::vector<uint32_t> &vertData, const std::vector<uint32_t> &fragData)
	{
		PROFILE_FUNCTION();

		auto vert = loadShader(vertData, ShaderType::Vertex);
		auto frag = loadShader(fragData, ShaderType::Fragment);
		//LOGV("{0} : {1}", shaderTypeToName(ShaderType::Vertex), vert);
		//LOGV("{0} : {1}", shaderTypeToName(ShaderType::Fragment), frag);

		GLCall(handle = glCreateProgram());
		std::vector<uint32_t> shaders;
		shaders.emplace_back(compile(vert, ShaderType::Vertex));
		shaders.emplace_back(compile(frag, ShaderType::Fragment));

		GLCall(glLinkProgram(handle));
		GLint result;
		GLCall(glGetProgramiv(handle, GL_LINK_STATUS, &result));
		if (result == GL_FALSE)
		{
			GLint length;
			GLCall(glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &length));
			std::string error;
			error.resize(length);
			GLCall(glGetProgramInfoLog(handle, length, &length, error.data()));
			LOGE("Failed to link shader : {0}", error);
		}
		GLCall(glValidateProgram(handle));

		for (auto &id : shaders)
		{
			GLCall(glDetachShader(handle, id));
			GLCall(glDeleteShader(id));
		}
	}

	GLShader::~GLShader()
	{
		PROFILE_FUNCTION();
		GLCall(glDeleteProgram(handle));
	}

	auto GLShader::init() -> void
	{
		PROFILE_FUNCTION();

		std::vector<std::string> lines;
		StringUtils::split(source, "\n", lines);
		std::unordered_map<ShaderType, std::string> sources;
		parseSource(lines, sources);

		GLCall(handle = glCreateProgram());
		std::vector<uint32_t> shaders;
		for (auto &source : sources)
		{
			auto buffer   = File::read(source.second);
			auto size     = buffer->size() / sizeof(uint32_t);
			auto glslCode = loadShader({reinterpret_cast<uint32_t *>(buffer->data()), reinterpret_cast<uint32_t *>(buffer->data()) + size}, source.first);
			//LOGV("\n{0} : {1}", shaderTypeToName(source.first), glslCode);
			shaders.emplace_back(compile(glslCode, source.first));
		}
		GLCall(glLinkProgram(handle));
		GLint result;
		GLCall(glGetProgramiv(handle, GL_LINK_STATUS, &result));
		if (result == GL_FALSE)
		{
			GLint length;
			GLCall(glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &length));
			std::string error;
			error.resize(length);
			GLCall(glGetProgramInfoLog(handle, length, &length, error.data()));

			LOGE("Failed to link shader : {0}", error);
			MAPLE_ASSERT(false, error);
		}
		GLCall(glValidateProgram(handle));

		for (auto &id : shaders)
		{
			GLCall(glDetachShader(handle, id));
			GLCall(glDeleteShader(id));
		}
	}
	auto GLShader::bind() const -> void
	{
		PROFILE_FUNCTION();
		if (shaderIndicator != this)
		{
			GLCall(glUseProgram(handle));
			shaderIndicator = this;
		}
	}
	auto GLShader::unbind() const -> void
	{
		PROFILE_FUNCTION();
		GLCall(glUseProgram(0));
		shaderIndicator = nullptr;
	}

	auto GLShader::bindPushConstants(const CommandBuffer *cmdBuffer, Pipeline *pipeline) -> void
	{
		PROFILE_FUNCTION();
		int index = 0;
		for (auto &constants : pushConstants)
		{
			for (auto &member : constants.members)
			{
				setUniform(member.type, constants.data.data(), member.size, member.offset, member.fullName);
			}
		}
	}

	auto GLShader::setUserUniformBuffer(ShaderType type, uint8_t *data, uint32_t size) -> void
	{
		PROFILE_FUNCTION();
	}
	auto GLShader::setUniform(const std::string &name, uint8_t *data) -> void
	{
		PROFILE_FUNCTION();
	}

	auto GLShader::setUniform(ShaderDataType type, const uint8_t *data, uint32_t size, uint32_t offset, const std::string &name) -> void
	{
		PROFILE_FUNCTION();

		uint32_t location = 0xFFFFFFFF;
		if (auto iter = uniformLocations.find(name); iter == uniformLocations.end())
		{
			location = glGetUniformLocation(handle, name.c_str());
			if (location != GL_INVALID_INDEX)
			{
				uniformLocations[name] = location;
			}
			else
			{
				LOGW("Invalid uniform location {0}", name);
			}
		}
		else
		{
			location = uniformLocations[name];
		}
		if (location != 0xFFFFFFFF)
		{
			switch (type)
			{
				case ShaderDataType::Float32:
					maple::setUniform1f(location, *reinterpret_cast<const float *>(&data[offset]));
					break;
				case ShaderDataType::Int:
				case ShaderDataType::Int32:
					maple::setUniform1i(location, *reinterpret_cast<const int32_t *>(&data[offset]));
					break;
				case ShaderDataType::UInt:
					maple::setUniform1ui(location, *reinterpret_cast<const uint32_t *>(&data[offset]));
					break;
				case ShaderDataType::Vec2:
					maple::setUniform2f(location, *reinterpret_cast<const glm::vec2 *>(&data[offset]));
					break;
				case ShaderDataType::Vec3:
					maple::setUniform3f(location, *reinterpret_cast<const glm::vec3 *>(&data[offset]));
					break;
				case ShaderDataType::Vec4:
					maple::setUniform4f(location, *reinterpret_cast<const glm::vec4 *>(&data[offset]));
					break;
				case ShaderDataType::Mat3:
					maple::setUniformMat3(location, *reinterpret_cast<const glm::mat3 *>(&data[offset]));
					break;
				case ShaderDataType::Mat4:
					maple::setUniformMat4Array(location, &data[offset], size / sizeof(glm::mat4));
					break;
				case ShaderDataType::Mat4Array:
					maple::setUniformMat4Array(location, &data[offset], size / sizeof(glm::mat4));
					break;
				default:
					MAPLE_ASSERT(false, "Unknown type!");
			}
		}
	}

	auto GLShader::getUniformLocation(const std::string &name) -> uint32_t
	{
		PROFILE_FUNCTION();
		uint32_t location = GL_INVALID_INDEX;
		if (uniformLocations.find(name) == uniformLocations.end())
		{
			location = glGetUniformLocation(handle, name.c_str());
			if (location != GL_INVALID_INDEX)
			{
				uniformLocations[name] = location;
			}
			else
			{
				LOGW("Invalid uniform location {0}", name);
				return GL_INVALID_INDEX;
			}
		}
		return uniformLocations[name];
	}
	auto GLShader::setUniform1f(const std::string &name, float value) -> void
	{
		PROFILE_FUNCTION();
		maple::setUniform1f(getUniformLocation(name), value);
	}
	auto GLShader::setUniform1fv(const std::string &name, float *value, int32_t count) -> void
	{
		PROFILE_FUNCTION();
		maple::setUniform1fv(getUniformLocation(name), value, count);
	}
	auto GLShader::setUniform1i(const std::string &name, int32_t value) -> void
	{
		PROFILE_FUNCTION();
		maple::setUniform1i(getUniformLocation(name), value);
	}
	auto GLShader::setUniform1ui(const std::string &name, uint32_t value) -> void
	{
		PROFILE_FUNCTION();
		maple::setUniform1ui(getUniformLocation(name), value);
	}
	auto GLShader::setUniform1iv(const std::string &name, int32_t *value, int32_t count) -> void
	{
		PROFILE_FUNCTION();
		maple::setUniform1iv(getUniformLocation(name), value, count);
	}
	auto GLShader::setUniform2f(const std::string &name, const glm::vec2 &value) -> void
	{
		PROFILE_FUNCTION();
		maple::setUniform2f(getUniformLocation(name), value);
	}
	auto GLShader::setUniform3f(const std::string &name, const glm::vec3 &value) -> void
	{
		PROFILE_FUNCTION();
		maple::setUniform3f(getUniformLocation(name), value);
	}
	auto GLShader::setUniform4f(const std::string &name, const glm::vec4 &value) -> void
	{
		PROFILE_FUNCTION();
		maple::setUniform4f(getUniformLocation(name), value);
	}
	auto GLShader::setUniformMat4(const std::string &name, const glm::mat4 &value) -> void
	{
		PROFILE_FUNCTION();
		maple::setUniformMat4(getUniformLocation(name), value);
	}

	auto GLShader::bindUniformBuffer(const GLUniformBuffer *buffer, uint32_t slot, const std::string &name) -> void
	{
		PROFILE_FUNCTION();
		const auto &location = uniformLocations.find(name);
		if (location != uniformLocations.end())
		{
			GLCall(glUniformBlockBinding(handle, location->second, slot));
		}
	}

	auto GLShader::getDescriptorInfo(uint32_t index) -> const std::vector<Descriptor>
	{
		if (descriptorInfos.find(index) != descriptorInfos.end())
		{
			return descriptorInfos[index];
		}
		LOGW("DescriptorSetInfo not found. Index = {0}", index);
		return std::vector<Descriptor>();
	}

	auto GLShader::loadShader(const std::vector<uint32_t> &spvCode, ShaderType type) -> std::string
	{
		PROFILE_FUNCTION();

		auto glsl = std::make_unique<spirv_cross::CompilerGLSL>(spvCode);

		spirv_cross::ShaderResources resources = glsl->get_shader_resources();

		if (type == ShaderType::Vertex)
		{
			uint32_t stride = 0;

			layout.getLayout().resize(resources.stage_inputs.size());

			for (const spirv_cross::Resource &resource : resources.stage_inputs)
			{
				const spirv_cross::SPIRType &inputType = glsl->get_type(resource.type_id);

				uint32_t localtion = glsl->get_decoration(resource.id, spv::DecorationLocation);

				initBufferLayout(inputType, layout, resource.name, localtion);

				stride += getStrideFromOpenGLFormat(0);
			}
			layout.computeStride();
		}

		if (type == ShaderType::Compute)
		{
			computeShader = true;
			localSizeX    = glsl->get_execution_mode_argument(spv::ExecutionMode::ExecutionModeLocalSize, 0);
			localSizeY    = glsl->get_execution_mode_argument(spv::ExecutionMode::ExecutionModeLocalSize, 1);
			localSizeZ    = glsl->get_execution_mode_argument(spv::ExecutionMode::ExecutionModeLocalSize, 2);
			//todo...reflect the image
		}

		for (auto &resource : resources.storage_images)
		{
			auto &glslType  = glsl->get_type(resource.base_type_id);
			auto &glslType2 = glsl->get_type(resource.type_id);

			if (glslType.basetype == spirv_cross::SPIRType::Image)
			{
				const char *fmt     = glsl->format_to_glsl(glslType.image.format);
				uint32_t    set     = glsl->get_decoration(resource.id, spv::DecorationDescriptorSet);
				uint32_t    binding = glsl->get_decoration(resource.id, spv::DecorationBinding);

				LOGV("Load Image, type {0}, set {1}, binding {2}, name {3}", fmt, set, binding, resource.name);

				auto &descriptorInfo = descriptorInfos[set];
				auto &descriptor     = descriptorInfo.emplace_back();

				descriptor.offset     = 0;
				descriptor.size       = 0;
				descriptor.binding    = binding;
				descriptor.name       = resource.name;
				descriptor.shaderType = type;
				descriptor.type       = DescriptorType::Image;
				//descriptor.accessFlag = glslType.image.access;
				descriptor.accessFlag = spv::AccessQualifierReadWrite;
				descriptor.format     = spirvTypeToTextureType(glslType.image.format);
			}
		}

		for (auto &resource : resources.sampled_images)
		{
			uint32_t set     = glsl->get_decoration(resource.id, spv::DecorationDescriptorSet);
			uint32_t binding = glsl->get_decoration(resource.id, spv::DecorationBinding);

			auto &descriptorInfo = descriptorInfos[set];
			auto &descriptor     = descriptorInfo.emplace_back();

			descriptor.offset     = 0;
			descriptor.size       = 0;
			descriptor.binding    = binding;
			descriptor.name       = resource.name;
			descriptor.shaderType = type;
			descriptor.type       = DescriptorType::ImageSampler;
		}

		for (auto const &uniformBuffer : resources.uniform_buffers)
		{
			auto set{glsl->get_decoration(uniformBuffer.id, spv::Decoration::DecorationDescriptorSet)};
			glsl->set_decoration(uniformBuffer.id, spv::Decoration::DecorationDescriptorSet, DESCRIPTOR_TABLE_INITIAL_SPACE + 2 * set);

			auto  binding     = glsl->get_decoration(uniformBuffer.id, spv::DecorationBinding);
			auto &bufferType  = glsl->get_type(uniformBuffer.type_id);
			auto  bufferSize  = glsl->get_declared_struct_size(bufferType);
			auto  memberCount = (int32_t) bufferType.member_types.size();

			auto &descriptorInfo  = descriptorInfos[set];
			auto &descriptor      = descriptorInfo.emplace_back();
			descriptor.binding    = binding;
			descriptor.size       = (uint32_t) bufferSize;
			descriptor.name       = uniformBuffer.name;
			descriptor.offset     = 0;
			descriptor.shaderType = type;
			descriptor.type       = DescriptorType::UniformBuffer;
			descriptor.buffer     = nullptr;

			for (int i = 0; i < memberCount; i++)
			{
				const auto  type       = glsl->get_type(bufferType.member_types[i]);
				const auto &memberName = glsl->get_member_name(bufferType.self, i);
				const auto  size       = glsl->get_declared_struct_member_size(bufferType, i);
				const auto  offset     = glsl->type_struct_member_offset(bufferType, i);

				auto uniformName = uniformBuffer.name + "." + memberName;

				auto &member    = descriptor.members.emplace_back();
				member.size     = (uint32_t) size;
				member.offset   = offset;
				member.type     = spirvTypeToDataType(type, size);
				member.fullName = uniformName;
				member.name     = memberName;
			}
		}

		for (auto &u : resources.push_constant_buffers)
		{
			auto &pushConstantType = glsl->get_type(u.type_id);
			auto  name             = glsl->get_name(u.id);

			auto ranges = glsl->get_active_buffer_ranges(u.id);

			uint32_t rangeSizes = 0;
			for (auto &range : ranges)
			{
				rangeSizes += uint32_t(range.range);
			}

			auto &bufferType  = glsl->get_type(u.base_type_id);
			auto  bufferSize  = glsl->get_declared_struct_size(bufferType);
			auto  memberCount = (int32_t) bufferType.member_types.size();

			auto &pushConst = pushConstants.emplace_back();

			pushConst.size        = rangeSizes;
			pushConst.shaderStage = type;
			pushConst.data.resize(rangeSizes);

			for (int i = 0; i < memberCount; i++)
			{
				const auto  type       = glsl->get_type(bufferType.member_types[i]);
				const auto &memberName = glsl->get_member_name(bufferType.self, i);
				const auto  size       = glsl->get_declared_struct_member_size(bufferType, i);
				const auto  offset     = glsl->type_struct_member_offset(bufferType, i);

				std::string uniformName = u.name + "." + memberName;

				auto &member    = pushConst.members.emplace_back();
				member.size     = (uint32_t) size;
				member.offset   = offset;
				member.type     = spirvTypeToDataType(type,size);
				member.fullName = uniformName;
				member.name     = memberName;
			}
		}

		int32_t imageCount[16]  = {0};
		int32_t bufferCount[16] = {0};

		for (int32_t i = 0; i < descriptorInfos.size(); i++)
		{
			auto &descriptorInfo = descriptorInfos[i];
			for (auto &descriptor : descriptorInfo)
			{
				if (descriptor.type == DescriptorType::ImageSampler)
				{
					imageCount[i]++;
					if (i > 0)
					{
						descriptor.binding = descriptor.binding + imageCount[i - 1];
					}
				}
				else if (descriptor.type == DescriptorType::UniformBuffer)
				{
					bufferCount[i]++;
					if (i > 0)
					{
						descriptor.binding = descriptor.binding + bufferCount[i - 1];
					}
				}
			}
		}

		spirv_cross::CompilerGLSL::Options options;
		options.version                              = 450;
		options.es                                   = false;
		options.vulkan_semantics                     = false;
		options.separate_shader_objects              = true;
		options.enable_420pack_extension             = false;
		options.emit_push_constant_as_uniform_buffer = false;
		glsl->set_common_options(options);

		// Compile to GLSL, ready to give to GL driver.
		shaderCompilers.emplace(type, std::move(glsl));
		return shaderCompilers[type]->compile();
	}

	auto GLShader::compile(const std::string &source, ShaderType type) -> uint32_t
	{
		PROFILE_FUNCTION();
		const char *cStr = source.c_str();
		GLCall(GLuint shader = glCreateShader(typeToGL(type)));
		GLCall(glShaderSource(shader, 1, &cStr, nullptr));
		GLCall(glCompileShader(shader));

		GLint result;
		GLCall(glGetShaderiv(shader, GL_COMPILE_STATUS, &result));
		if (result == GL_FALSE)
		{
			GLint length;
			GLCall(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length));
			std::string error;
			error.resize(length);
			GLCall(glGetShaderInfoLog(shader, length, &length, error.data()));
			LOGE("Failed to compile {0} shader. Error : {1}", shaderTypeToName(type), error);
			GLCall(glDeleteShader(shader));
			return 0;
		}

		GLCall(glAttachShader(handle, shader));
		return shader;
	}

}        // namespace maple
