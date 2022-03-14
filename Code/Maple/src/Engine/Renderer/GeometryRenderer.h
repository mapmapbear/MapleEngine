//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Math/Frustum.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <memory>
#include "Scene/System/ExecutePoint.h"

namespace maple
{
	class BoundingBox;

	namespace component 
	{
		class Light;
	};

	namespace geometry_renderer 
	{
		auto registerGeometryRenderer(ExecuteQueue& begin, ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> executePoint) -> void;
	}

	class MAPLE_EXPORT GeometryRenderer
	{
	public:
		static auto drawLine(const glm::vec3 &start, const glm::vec3 &end, const glm::vec4 &color = {1.0f, 1.0f, 1.0f, 1.0f}) -> void;
		static auto drawFrustum(const Frustum &frustum) -> void;
		static auto drawRect(int32_t x, int32_t y, int32_t width, int32_t height) -> void;
		static auto drawBox(const BoundingBox &box, const glm::vec4 &color) -> void;
		static auto drawTriangle(const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2, const glm::vec4 &color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)) -> void;
		static auto drawLight(component::Light* light, const glm::quat& rotation, const glm::vec4& color) -> void;
		static auto drawCone(int32_t numCircleVerts, int32_t numLinesToCircle, float angle, float length, const glm::vec3& position, const glm::quat& rotation, const glm::vec4& color) -> void;
		static auto drawSphere(float radius, const glm::vec3& position, const glm::vec4& color) -> void;
		static auto drawCircle(int32_t numVerts, float radius, const glm::vec3& position, const glm::quat& rotation, const glm::vec4& color) -> void;
	};
};        // namespace maple
