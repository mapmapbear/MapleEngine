#pragma once

#include "RHI/BufferLayout.h"
#include "RHI/Shader.h"
#include <array>
#include <glm/glm.hpp>
#include <map>
#include <spirv_glsl.hpp>
#include <string>
#include <unordered_map>

namespace maple
{
	class GLUniformBuffer;

	struct GLShaderErrorInfo
	{
		uint32_t    shader = 0;
		uint32_t    line[6];
		std::string message[6];
	};

	class GLShader : public Shader
	{
	  private:
		friend class Shader;
		friend class ShaderManager;

	  public:
		GLShader(const std::string &filePath, bool loadSPV = false);
		GLShader(const std::vector<uint32_t> &vertData, const std::vector<uint32_t> &fragData);

		~GLShader();

		auto reload() -> void override;

		auto init() -> void;
		auto bind() const -> void override;
		auto unbind() const -> void override;
		auto bindPushConstants(CommandBuffer *cmdBuffer, Pipeline *pipeline) -> void override;

		auto setUserUniformBuffer(ShaderType type, uint8_t *data, uint32_t size) -> void;
		auto setUniform(const std::string &name, uint8_t *data) -> void;
		auto setUniform(ShaderDataType type, const uint8_t *data, uint32_t size, uint32_t offset, const std::string &name) -> void;

		auto getUniformLocation(const std::string &name) -> uint32_t;
		auto setUniform1f(const std::string &name, float value) -> void;
		auto setUniform1fv(const std::string &name, float *value, int32_t count) -> void;
		auto setUniform1i(const std::string &name, int32_t value) -> void;
		auto setUniform1ui(const std::string &name, uint32_t value) -> void;
		auto setUniform1iv(const std::string &name, int32_t *value, int32_t count) -> void;
		auto setUniform2f(const std::string &name, const glm::vec2 &vector) -> void;
		auto setUniform3f(const std::string &name, const glm::vec3 &vector) -> void;
		auto setUniform4f(const std::string &name, const glm::vec4 &vector) -> void;
		auto setUniformMat4(const std::string &name, const glm::mat4 &matrix) -> void;
		auto bindUniformBuffer(const GLUniformBuffer *buffer, uint32_t slot, const std::string &name) -> void;

		inline auto getPushConstants() -> std::vector<PushConstant> & override
		{
			return pushConstants;
		}

		inline auto getHandle() const -> void * override
		{
			return (void *) (size_t) handle;
		}

		inline auto &getLayout() const
		{
			return layout;
		}

		inline auto getName() const -> const std::string & override
		{
			return name;
		}

		inline auto getFilePath() const -> const std::string & override
		{
			return filePath;
		}

		auto getDescriptorInfo(uint32_t index) -> const DescriptorSetInfo override;

		inline auto getProgramId() const
		{
			return handle;
		}

		inline auto getPath() const->std::string override
		{
			return filePath;
		};

		inline auto getLocalSizeX() const -> uint32_t override
		{
			return localSizeX;
		};
		inline auto getLocalSizeY() const -> uint32_t override
		{
			return localSizeY;
		};
		inline auto getLocalSizeZ() const -> uint32_t override
		{
			return localSizeZ;
		};

	  private:
		//spv -> glsl code
		auto loadShader(const std::vector<uint32_t> &spvCode, ShaderType type) -> std::string;
		auto compile(const std::string &source, ShaderType type) -> uint32_t;

		bool        loadSPV = false;
		uint32_t    handle  = -1;
		std::string name;
		std::string source;

		std::unordered_map<uint32_t, DescriptorSetInfo> descriptorInfos;
		std::unordered_map<std::string, uint32_t>       uniformLocations;
		std::unordered_map<uint32_t, uint32_t>          sampledLocations;
		std::unordered_map<ShaderType, std::string>     sources;

		std::unordered_map<ShaderType, std::unique_ptr<spirv_cross::CompilerGLSL>> shaderCompilers;
		std::array<std::string, static_cast<size_t>(ShaderType::Length)>           shaderSources;

		BufferLayout              layout;
		std::vector<PushConstant> pushConstants;
		const std::string         filePath;

		uint32_t localSizeX = 1;
		uint32_t localSizeY = 1;
		uint32_t localSizeZ = 1;

		bool computeShader = false;
	};
}        // namespace maple
