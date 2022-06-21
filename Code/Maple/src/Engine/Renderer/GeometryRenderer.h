//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <memory>

#include "Engine/Core.h"
#include "Math/Frustum.h"
#include "Scene/System/ExecutePoint.h"

namespace maple
{
	class BoundingBox;

	namespace component
	{
		struct Light;
	};

	namespace geometry_renderer
	{
		auto registerGeometryRenderer(ExecuteQueue &begin, ExecuteQueue &renderer, std::shared_ptr<ExecutePoint> executePoint) -> void;
	}

	namespace GeometryRenderer
	{
		auto MAPLE_EXPORT drawLine(const glm::vec3 &start, const glm::vec3 &end, const glm::vec4 &color = {1.0f, 1.0f, 1.0f, 1.0f}) -> void;
		auto MAPLE_EXPORT drawFrustum(const Frustum &frustum, const glm::vec4 &color = {1.0f, 1.0f, 1.0f, 1.0f}) -> void;
		auto MAPLE_EXPORT drawRect(int32_t x, int32_t y, int32_t width, int32_t height) -> void;
		auto MAPLE_EXPORT drawBox(const glm::vec3 &position, const BoundingBox &box, const glm::vec4 &color) -> void;
		auto MAPLE_EXPORT drawTriangle(const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2, const glm::vec4 &color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)) -> void;
		auto MAPLE_EXPORT drawLight(component::Light *light, const glm::quat &rotation, const glm::vec4 &color) -> void;
		auto MAPLE_EXPORT drawCone(int32_t numCircleVerts, int32_t numLinesToCircle, float angle, float length, const glm::vec3 &position, const glm::quat &rotation, const glm::vec4 &color, bool backward = false) -> void;
		auto MAPLE_EXPORT drawSphere(float radius, const glm::vec3 &position, const glm::vec4 &color) -> void;
		auto MAPLE_EXPORT drawCircle(int32_t numVerts, float radius, const glm::vec3 &position, const glm::quat &rotation, const glm::vec4 &color) -> void;
		auto MAPLE_EXPORT drawCapsule(const glm::vec3 &position, const glm::quat &rotation, float height, float radius, const glm::vec4 &color) -> void;
		auto MAPLE_EXPORT drawArc(int32_t numVerts, float radius, const glm::vec3 &start, const glm::vec3 &end, const glm::quat &rotation, const glm::vec4 &color) -> void;
	};        // namespace GeometryRenderer
};            // namespace maple
