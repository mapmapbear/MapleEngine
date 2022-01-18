#include "CloudRenderer.h"
#include "RHI/Pipeline.h"
#include "RHI/Shader.h"
#include "RHI/Texture.h"

#include "Engine/Profiler.h"
#include "Scene/Component/VolumetricCloud.h"
#include "Scene/Scene.h"

namespace maple
{
	enum CloudsTextures
	{
		FragColor,
		Bloom,
		Alphaness,
		CloudDistance,
		Length
	};

	struct CloudRenderer::RenderData
	{
		std::shared_ptr<Shader>                 cloudShader;
		std::vector<std::shared_ptr<Texture2D>> computeInputs;
		std::shared_ptr<DescriptorSet>          descriptorSet;
		RenderData()
		{
			cloudShader = Shader::create("shaders/Cloud.shader");
			for (int32_t i = 0; i < CloudsTextures::Length; i++)
			{
				computeInputs.emplace_back(Texture2D::create());
			}
			descriptorSet = DescriptorSet::create({0, cloudShader.get()});
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
		PROFILE_FUNCTION();
		PipelineInfo info;

		auto pipeline = Pipeline::get(info);

		pipeline->bind(getCommandBuffer());
		Renderer::bindDescriptorSets(pipeline.get(), getCommandBuffer(), 0, {data->descriptorSet});
		Renderer::dispatch(getCommandBuffer(), 1,1,1);
		pipeline->end(getCommandBuffer());
	}

	auto CloudRenderer::beginScene(Scene *scene, const glm::mat4 &projView) -> void
	{
		auto view = scene->getRegistry().view<VolumetricCloud>();
		if (view.size() > 0)
		{
			auto &cloud = scene->getRegistry().get<VolumetricCloud>(view[0]);
		}
	}

	auto CloudRenderer::onResize(uint32_t width, uint32_t height) -> void
	{
		for (auto &input : data->computeInputs)
		{
			input->buildTexture(TextureFormat::RGBA32, width, height);
		}
	}
};        // namespace maple
