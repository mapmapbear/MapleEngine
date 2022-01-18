#include "CloudRenderer.h"
#include "RHI/Shader.h"
#include "Scene/Component/VolumetricCloud.h"
#include "Scene/Scene.h"

namespace maple
{
	struct CloudRenderer::RenderData
	{
		std::shared_ptr<Shader> cloudShader;
		RenderData()
		{
			cloudShader = Shader::create("shaders/Cloud.shader");
		}
	};

	CloudRenderer::~CloudRenderer()
	{
		delete data;
	}

	auto CloudRenderer::init(const std::shared_ptr<GBuffer> &buffer) -> void
	{
		data    = new RenderData();
		gbuffer = buffer;
	}

	auto CloudRenderer::renderScene() -> void
	{
	}

	auto CloudRenderer::beginScene(Scene *scene, const glm::mat4 &projView) -> void
	{
		auto view = scene->getRegistry().view<VolumetricCloud>();
		if (view.size() > 0)
		{
			auto &cloud = scene->getRegistry().get<VolumetricCloud>(view[0]);
		}
	}
};        // namespace maple
