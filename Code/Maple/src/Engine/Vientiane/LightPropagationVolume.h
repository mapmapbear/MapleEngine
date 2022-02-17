//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Engine/Renderer/Renderer.h"

namespace maple
{
	class LightPropagationVolume : public Renderer
	{
	  public:
		auto init(const std::shared_ptr<GBuffer> &buffer) -> void override;
		auto renderScene(Scene *scene) -> void override;
		auto beginScene(Scene *scene, const glm::mat4 &projView) -> void override;

		struct RendererData;

		RendererData *renderData = nullptr;
	};
};        // namespace maple
