//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Quad2D.h"
#include "Engine/Vertex.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "Renderer.h"
#include "RHI/DescriptorSet.h"

namespace maple
{
	/*class UniformBuffer;
	class Texture2D;
	class VertexBuffer;
	class IndexBuffer;

	struct Config2D
	{
		uint32_t maxQuads          = 10000;
		uint32_t quadsSize         = sizeof(Vertex2D) * 4;
		uint32_t bufferSize        = 10000 * sizeof(Vertex2D) * 4;
		uint32_t indiciesSize      = 10000 * 6;
		uint32_t maxTextures       = 16;
		uint32_t maxBatchDrawCalls = 10;

		auto setMaxQuads(uint32_t quads) -> void
		{
			maxQuads     = quads;
			bufferSize   = maxQuads * sizeof(Vertex2D) * 4;
			indiciesSize = maxQuads * 6;
		}
	};

	struct Triangle
	{
		glm::vec3 pos1;
		glm::vec3 pos2;
		glm::vec3 pos3;
		glm::vec4 color;
	};

	struct Command2D
	{
		const Quad2D *quad;
		glm::mat4     transform;
	};*/

};        // namespace maple
