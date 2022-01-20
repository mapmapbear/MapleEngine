#include "CloudRenderer.h"
#include "RHI/Pipeline.h"
#include "RHI/Shader.h"
#include "RHI/Texture.h"

#include "Engine/Camera.h"
#include "Engine/GBuffer.h"
#include "Engine/Profiler.h"
#include "Scene/Component/Light.h"
#include "Scene/Component/Transform.h"
#include "Scene/Component/VolumetricCloud.h"
#include "Scene/Scene.h"

#include "Application.h"

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

	struct UniformBufferObject
	{
		glm::mat4 invView;
		glm::mat4 invProj;
		glm::mat4 invViewProj;

		glm::vec3 lightColor;
		glm::vec3 lightDirection;
		glm::vec3 cameraPosition;

		glm::vec3 skyColorTop    = glm::vec3(0.5, 0.7, 0.8) * 1.05f;
		glm::vec3 skyColorBottom = glm::vec3(0.9, 0.9, 0.95);

		float FOV;
		float iTime;
		float coverageMultiplier;
		float cloudSpeed;
		float crispiness;

		float earthRadius;
		float sphereInnerRadius;
		float sphereOuterRadius;
		float curliness;
		float absorption;
		float densityFactor;
		int   enablePowder = 0;
		float padding1;
	};

	struct CloudRenderer::RenderData
	{
		std::shared_ptr<Shader>                 cloudShader;
		std::vector<std::shared_ptr<Texture2D>> computeInputs;
		std::shared_ptr<DescriptorSet>          descriptorSet;
		std::shared_ptr<Pipeline>               pipeline;
		UniformBufferObject                     uniformObject;
		RenderData()
		{
			cloudShader   = Shader::create("shaders/Cloud.shader");
			descriptorSet = DescriptorSet::create({0, cloudShader.get()});
			memset(&uniformObject, 0, sizeof(UniformBufferObject));
		}
	};

	CloudRenderer::CloudRenderer()
	{
	}

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
		if (data->pipeline)
		{
			data->descriptorSet->setTexture("fragColor", data->computeInputs[0]);
			data->descriptorSet->setTexture("bloom", data->computeInputs[1]);
			data->descriptorSet->setTexture("alphaness", data->computeInputs[2]);
			data->descriptorSet->setTexture("cloudDistance", data->computeInputs[3]);
			data->descriptorSet->update();

			data->pipeline->bind(getCommandBuffer());
			Renderer::bindDescriptorSets(data->pipeline.get(), getCommandBuffer(), 0, {data->descriptorSet});
			Renderer::dispatch(getCommandBuffer(),
			                   gbuffer->getWidth() / data->cloudShader->getLocalSizeX(),
			                   gbuffer->getHeight() / data->cloudShader->getLocalSizeY(),
			                   1);
			data->pipeline->end(getCommandBuffer());
		}
	}

	auto CloudRenderer::beginScene(Scene *scene, const glm::mat4 &projView) -> void
	{
		auto group = scene->getRegistry().group<VolumetricCloud>(entt::get<Light, Transform>);

		auto &camera = scene->getCamera();

		for (auto entity : group)
		{
			auto [cloud, light, transform] = group.get<VolumetricCloud, Light, Transform>(entity);

			auto         timer = Application::getTimer().currentTimestamp() / 1000.f;
			PipelineInfo info;
			info.shader      = data->cloudShader;
			info.groupCountX = data->computeInputs[0]->getWidth() / 16;
			info.groupCountY = data->computeInputs[0]->getWidth() / 16;
			//data->pipeline   = Pipeline::get(info);

			data->uniformObject.invProj        = glm::inverse(camera.first->getProjectionMatrix());
			data->uniformObject.invView        = camera.second->getWorldMatrix();
			data->uniformObject.cameraPosition = camera.second->getWorldPosition();
			data->uniformObject.lightDirection = glm::normalize(transform.getWorldOrientation() * maple::FORWARD);

			data->uniformObject.FOV   = camera.first->getFov();
			data->uniformObject.iTime = timer;

			data->uniformObject.earthRadius        = cloud.earthRadius;
			data->uniformObject.sphereInnerRadius  = cloud.sphereInnerRadius;
			data->uniformObject.sphereOuterRadius  = cloud.sphereOuterRadius;
			data->uniformObject.coverageMultiplier = cloud.coverage;
			data->uniformObject.cloudSpeed         = cloud.cloudSpeed;
			data->uniformObject.curliness          = cloud.curliness;
			data->uniformObject.absorption         = cloud.absorption * 0.1;
			data->uniformObject.densityFactor      = cloud.density;
			data->pipeline                         = Pipeline::get(info);
		}
	}

	auto CloudRenderer::onResize(uint32_t width, uint32_t height) -> void
	{
		if (data->computeInputs.empty())
		{
			for (auto i = 0; i < data->cloudShader->getDescriptorInfo(0).descriptors.size(); i++)
			{
				data->computeInputs.emplace_back(Texture2D::create());
			}
		}
		for (auto &desc : data->cloudShader->getDescriptorInfo(0).descriptors)
		{
			auto &binding = data->computeInputs[desc.binding];
			binding->buildTexture(TextureFormat::RGBA32, width, height, false, true, false, false, true, desc.accessFlag);
		}
	}
};        // namespace maple
