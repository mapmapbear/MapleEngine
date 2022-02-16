//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include "Plane.h"
#include <algorithm>
#include <glm/glm.hpp>
#include <string>
namespace maple
{
	class MAPLE_EXPORT Frustum
	{
	  public:
		enum FrustumPlane
		{
			PlaneNear = 0,
			PlaneLeft,
			PlaneRight,
			PlaneUp,
			PlaneDown,
			PlaneFar
		};

		friend class RenderGraph;
		friend class GeometryRenderer;
		static constexpr uint32_t FRUSTUM_VERTICES = 8;

		Frustum() noexcept = default;

		auto from(const glm::mat4 &projection) -> void;

		auto isInside(const glm::vec3 &pos) const -> bool;

		inline auto &getPlane(FrustumPlane id) const
		{
			return planes[id];
		}

		inline auto getVertices() -> glm::vec3 *
		{
			return vertices;
		}

	  private:
		Plane     planes[6];
		glm::vec3 vertices[FRUSTUM_VERTICES];
	};

};        // namespace maple
