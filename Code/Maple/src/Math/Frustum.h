//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <string>
#include <glm/glm.hpp>
#include <algorithm>
#include "Engine/Core.h"
namespace maple
{

	class MAPLE_EXPORT Frustum
	{
	public:

		friend class RenderGraph;
		friend class GeometryRenderer;
		static constexpr uint32_t FRUSTUM_VERTICES = 8;

		Frustum() noexcept = default;
		
		auto from(const glm::mat4 & projection) -> void;

	private:
		glm::vec3 vertices[FRUSTUM_VERTICES];
	};

};