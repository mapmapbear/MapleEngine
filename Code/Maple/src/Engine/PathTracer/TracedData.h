//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <cstdint>
#include <glm/glm.hpp>

namespace maple::raytracing
{
	struct MaterialData
	{
		glm::ivec4 textureIndices0 = glm::ivec4(-1);        // x: albedo, y: normals, z: roughness, w: metallic
		glm::ivec4 textureIndices1 = glm::ivec4(-1);        // x: emissive, y: ao
		glm::vec4  albedo;
		glm::vec4  roughness;
		glm::vec4  metalic;
		glm::vec4  emissive;        // w -> workflow
	};

	struct TransformData
	{
		glm::mat4 model;
		uint32_t  meshIndex;
		float     padding[3];
	};

}        // namespace maple::raytracing