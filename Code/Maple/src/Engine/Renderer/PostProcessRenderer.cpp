//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "PostProcessRenderer.h"
#include "Engine/Profiler.h"
#include "RHI/Shader.h"

namespace maple
{
	struct PostProcessRenderer::RenderData
	{

		RenderData()
		{
		
		}
	};

	PostProcessRenderer::PostProcessRenderer()
	{
		data = new PostProcessRenderer::RenderData();
	}

	PostProcessRenderer::~PostProcessRenderer()
	{
		delete data;
	}

	auto PostProcessRenderer::init(const std::shared_ptr<GBuffer> &buffer) -> void
	{
		PROFILE_FUNCTION();
	}

	auto PostProcessRenderer::renderScene() -> void
	{
		PROFILE_FUNCTION();
	}

	auto PostProcessRenderer::beginScene(Scene *scene, const glm::mat4 &projView) -> void
	{
		PROFILE_FUNCTION();
	}

	auto PostProcessRenderer::onResize(uint32_t width, uint32_t height) -> void
	{
		PROFILE_FUNCTION();
	}
};        // namespace maple
