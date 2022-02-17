//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "LightPropagationVolume.h"

namespace maple
{
	struct LightPropagationVolume::RendererData
	{
	};

	auto LightPropagationVolume::init(const std::shared_ptr<GBuffer> &buffer) -> void
	{
		gbuffer    = buffer;
		renderData = new RendererData();
	}

	auto LightPropagationVolume::renderScene(Scene *scene) -> void
	{
	}

	auto LightPropagationVolume::beginScene(Scene *scene, const glm::mat4 &projView) -> void
	{
	}
};        // namespace maple
