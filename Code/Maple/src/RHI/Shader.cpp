//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "Shader.h"
#include <spirv_cross.hpp>
#include "Others/StringUtils.h"
#include "Others/Console.h"

namespace Maple
{
	namespace
	{
		auto getShaderTypeByName(const std::string &name) -> ShaderType
		{
			if (StringUtils::contains(name, "Vertex"))
			{
				return ShaderType::Vertex;
			}

			if (StringUtils::contains(name, "Fragment"))
			{
				return ShaderType::Fragment;
			}

			if (StringUtils::contains(name, "Geometry"))
			{
				return ShaderType::Geometry;
			}
			if (StringUtils::contains(name, "Compute"))
			{
				return ShaderType::Compute;
			}
			return ShaderType::Unknown;
		}
	}


	auto Shader::spirvTypeToDataType(const spirv_cross::SPIRType &type) -> ShaderDataType
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
		return ShaderDataType::None;
	}

	auto Shader::parseSource(const std::vector<std::string> &lines, std::unordered_map<ShaderType, std::string> &shaders) -> void
	{
		for (uint32_t i = 0; i < lines.size(); i++)
		{
			std::string str = lines[i];
			if (StringUtils::startWith(str, "#"))
			{
				auto path = StringUtils::split(str, " ");
				auto type = getShaderTypeByName(path[0]);
				if (type != ShaderType::Unknown)
				{
					StringUtils::trim(path[1], "\r");
					shaders[type] = path[1];
					LOGV("{0} : {1}",path[0],path[1]);
				}
			}
		}
	}

}        // namespace Maple
