//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Math/Frustum.h"
#include "Renderer.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace maple
{
	class Texture;
	class Scene;
	class BoundingBox;
	class Transform;

	struct GeometryLine
	{
		glm::vec3 start;
		glm::vec3 end;
		glm::vec4 color;
	};

	struct GeometryPoint
	{
		glm::vec3 p1;
		glm::vec4 color;
		float     size;
	};

	struct GeometryTriangle
	{
		glm::vec3 p1;
		glm::vec3 p2;
		glm::vec3 p3;
		glm::vec4 color;
	};

	class MAPLE_EXPORT GeometryRenderer : public Renderer
	{
	  public:
		auto init(const std::shared_ptr<GBuffer> &buffer) -> void override;
		auto renderScene(Scene *scene) -> void override;
		auto beginScene(Scene *scene, const glm::mat4 &projView) -> void override;

		static auto drawLine(const glm::vec3 &start, const glm::vec3 &end, const glm::vec4 &color = {1.0f, 1.0f, 1.0f, 1.0f}) -> void;
		static auto drawFrustum(const Frustum &frustum) -> void;
		static auto drawRect(int32_t x, int32_t y, int32_t width, int32_t height) -> void;
		static auto drawBox(const BoundingBox &box, const glm::vec4 &color) -> void;
		static auto drawTriangle(const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2, const glm::vec4 &color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)) -> void;

		inline auto &getLines()
		{
			return lines;
		}

		inline auto &getTriangles()
		{
			return triangles;
		}

		inline auto &getPoints()
		{
			return points;
		}

		auto executeLinePass() -> void;
		auto executePointPass() -> void;

	  private:
		struct RenderData;

		std::vector<GeometryLine>     lines;
		std::vector<GeometryTriangle> triangles;
		std::vector<GeometryPoint>    points;
		std::shared_ptr<RenderData>   renderData;
		Transform *                   transform = nullptr;
	};
};        // namespace maple
