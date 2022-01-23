//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Renderer.h"

namespace maple
{
	class SkyboxRenderer : public Renderer
	{
	  public:
		SkyboxRenderer();
		virtual ~SkyboxRenderer();
		auto init(const std::shared_ptr<GBuffer> &buffer) -> void override;
		auto renderScene() -> void override;
		auto beginScene(Scene *scene, const glm::mat4 &projView) -> void override;
		auto onResize(uint32_t width, uint32_t height) -> void override;

	  private:
		auto onImGui() -> void override;
		struct SkyboxData;
		SkyboxData *skyboxData = nullptr;
	};
}        // namespace maple
