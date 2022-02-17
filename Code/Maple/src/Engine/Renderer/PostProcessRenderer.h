//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Renderer.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace maple
{
	class MAPLE_EXPORT PostProcessRenderer : public Renderer
	{
	  public:
		PostProcessRenderer();

		~PostProcessRenderer();

		auto init(const std::shared_ptr<GBuffer> &buffer) -> void override;

		auto renderScene(Scene *scene) -> void override;

		auto beginScene(Scene *scene, const glm::mat4 &projView) -> void override;

		auto onResize(uint32_t width, uint32_t height) -> void override;

	  private:
		struct RenderData;
		RenderData *data = nullptr;
	};
}        // namespace maple
