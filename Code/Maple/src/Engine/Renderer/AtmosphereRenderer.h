//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Renderer.h"

namespace maple
{
	class AtmosphereRenderer : public Renderer
	{
	  public:
		AtmosphereRenderer();
		virtual ~AtmosphereRenderer();
		auto init(const std::shared_ptr<GBuffer> &buffer) -> void override;
		auto renderScene() -> void override;
		auto beginScene(Scene *scene, const glm::mat4 &projView) -> void override;
		auto onImGui() -> void;
	  private:
		struct RenderData;
		RenderData *data = nullptr;
	};
}        // namespace maple
