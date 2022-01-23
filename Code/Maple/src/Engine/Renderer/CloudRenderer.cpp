#include "CloudRenderer.h"
#include "RHI/Pipeline.h"
#include "RHI/Shader.h"
#include "RHI/Texture.h"

#include "Engine/Camera.h"
#include "Engine/GBuffer.h"
#include "Engine/Mesh.h"
#include "Engine/Profiler.h"

#include "Scene/Component/Light.h"
#include "Scene/Component/Transform.h"
#include "Scene/Component/VolumetricCloud.h"
#include "Scene/Scene.h"

#include "Others/Randomizer.h"

#include "ImGui/ImGuiHelpers.h"

#include "Application.h"

namespace maple
{
	struct UniformBufferObject
	{
		glm::mat4 invView;
		glm::mat4 invProj;

		glm::vec4 lightColor;
		glm::vec4 lightDirection;
		glm::vec4 cameraPosition;

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
		float steps;

		float   padding1;
		float   padding2;
		float   padding3;
		int32_t enablePowder;
	};

	struct CloudRenderer::RenderData
	{
		std::shared_ptr<Shader>                 cloudShader;
		std::shared_ptr<Pipeline>               pipeline;
		std::shared_ptr<DescriptorSet>          descriptorSet;
		std::vector<std::shared_ptr<Texture2D>> computeInputs;
		UniformBufferObject                     uniformObject;

		std::shared_ptr<Shader>        screenCloudShader;
		std::shared_ptr<DescriptorSet> screenDescriptorSet;
		std::shared_ptr<Mesh>          screenMesh;

		RenderData()
		{
			cloudShader   = Shader::create("shaders/Cloud.shader");
			descriptorSet = DescriptorSet::create({0, cloudShader.get()});
			memset(&uniformObject, 0, sizeof(UniformBufferObject));
			uniformObject.steps = 64;

			screenCloudShader   = Shader::create("shaders/CloudScreen.shader");
			screenDescriptorSet = DescriptorSet::create({0, screenCloudShader.get()});
			screenMesh          = Mesh::createQuad(true);
		}
	};

	struct CloudRenderer::WeatherPass
	{
		std::shared_ptr<Shader>        shader;
		std::shared_ptr<DescriptorSet> descriptorSet;
		std::shared_ptr<Texture2D>     weather;

		std::shared_ptr<Shader>        perlinWorley;
		std::shared_ptr<Texture3D>     perlin3D;
		std::shared_ptr<DescriptorSet> perlinWorleySet;

		std::shared_ptr<Shader>        worleyShader;
		std::shared_ptr<Texture3D>     worley3D;
		std::shared_ptr<DescriptorSet> worleySet;

		struct UniformBufferObject
		{
			glm::vec3 seed;
			float     perlinAmplitude = 0.5;
			float     perlinFrequency = 0.8;
			float     perlinScale     = 100.0;
			int       perlinOctaves   = 4;
		} uniformObject;

		WeatherPass()
		{
			shader        = Shader::create("shaders/Weather.shader");
			descriptorSet = DescriptorSet::create({0, shader.get()});
			weather       = Texture2D::create();
			weather->buildTexture(TextureFormat::RGBA32, 1024, 1024, false, true, false, false, true, 2);
			uniformObject.seed = {Randomizer::random(), Randomizer::random(), Randomizer::random()};

			perlinWorley    = Shader::create("shaders/PerlinWorley.shader");
			perlin3D        = Texture3D::create(128, 128, 128);
			perlinWorleySet = DescriptorSet::create({0, perlinWorley.get()});
			perlinWorleySet->setTexture("outVolTex", perlin3D);

			worleyShader = Shader::create("shaders/Worley.shader");
			worley3D     = Texture3D::create(32, 32, 32);
			worleySet    = DescriptorSet::create({0, worleyShader.get()});
			worleySet->setTexture("outVolTex", worley3D);
		}

		inline auto executePerlin3D(CommandBuffer *cmd)
		{
			PipelineInfo info;
			info.shader   = perlinWorley;
			auto pipeline = Pipeline::get(info);
			perlinWorleySet->update();
			pipeline->bind(cmd);
			Renderer::bindDescriptorSets(pipeline.get(), cmd, 0, {perlinWorleySet});
			Renderer::dispatch(cmd, 128 / 4, 128 / 4, 128 / 4);
			pipeline->end(cmd);
			perlin3D->generateMipmaps();
		}

		inline auto executeWorley3D(CommandBuffer *cmd)
		{
			PipelineInfo info;
			info.shader   = worleyShader;
			auto pipeline = Pipeline::get(info);
			worleySet->update();
			pipeline->bind(cmd);
			Renderer::bindDescriptorSets(pipeline.get(), cmd, 0, {worleySet});
			Renderer::dispatch(cmd, 32 / 4, 32 / 4, 32 / 4);
			pipeline->end(cmd);
			worley3D->generateMipmaps();
		}

		inline auto execute(CommandBuffer *cmd)
		{
			descriptorSet->setUniformBufferData("UniformBufferObject", &uniformObject);
			descriptorSet->setTexture("outWeatherTex", weather);
			descriptorSet->update();

			PipelineInfo info;
			info.shader   = shader;
			auto pipeline = Pipeline::get(info);
			pipeline->bind(cmd);
			Renderer::bindDescriptorSets(pipeline.get(), cmd, 0, {descriptorSet});
			Renderer::dispatch(cmd, 1024 / 8, 1024 / 8, 1);
			pipeline->end(cmd);
		}
	};

	CloudRenderer::CloudRenderer()
	{
	}

	CloudRenderer::~CloudRenderer()
	{
		delete data;
		delete skyData;
		delete weatherPass;
	}

	auto CloudRenderer::init(const std::shared_ptr<GBuffer> &buffer) -> void
	{
		data        = new RenderData();
		weatherPass = new WeatherPass();
		gbuffer     = buffer;

		weatherPass->executePerlin3D(getCommandBuffer());
		weatherPass->executeWorley3D(getCommandBuffer());
	}

	auto CloudRenderer::renderScene() -> void
	{
		PROFILE_FUNCTION();

		if (data->pipeline)
		{
			{
				data->descriptorSet->setUniformBufferData("UniformBufferObject", &data->uniformObject);
				data->descriptorSet->setTexture("fragColor", data->computeInputs[0]);
				data->descriptorSet->setTexture("bloom", data->computeInputs[1]);
				data->descriptorSet->setTexture("alphaness", data->computeInputs[2]);
				data->descriptorSet->setTexture("cloudDistance", data->computeInputs[3]);
				data->descriptorSet->setTexture("uDepthSampler", gbuffer->getDepthBuffer());
				data->descriptorSet->setTexture("uSky", gbuffer->getBuffer(GBufferTextures::PSEUDO_SKY));

				data->descriptorSet->setTexture("uWeatherTex", weatherPass->weather);
				data->descriptorSet->setTexture("uCloud", weatherPass->perlin3D);
				data->descriptorSet->setTexture("uWorley32", weatherPass->worley3D);

				data->descriptorSet->update();

				data->pipeline->bind(getCommandBuffer());
				Renderer::bindDescriptorSets(data->pipeline.get(), getCommandBuffer(), 0, {data->descriptorSet});
				Renderer::dispatch(getCommandBuffer(),
				                   gbuffer->getWidth() / data->cloudShader->getLocalSizeX(),
				                   gbuffer->getHeight() / data->cloudShader->getLocalSizeY(),
				                   1);
				data->pipeline->end(getCommandBuffer());
			}

			{
				data->screenDescriptorSet->update();
				PipelineInfo info;
				info.shader              = data->screenCloudShader;
				info.colorTargets[0]     = gbuffer->getBuffer(GBufferTextures::SCREEN);
				info.polygonMode         = PolygonMode::Fill;
				info.clearTargets        = false;
				info.transparencyEnabled = false;

				auto pipeline = Pipeline::get(info);

				data->screenDescriptorSet->setTexture("uCloudSampler", data->computeInputs[0]);
				data->screenDescriptorSet->update();

				pipeline->bind(getCommandBuffer());
				Renderer::bindDescriptorSets(pipeline.get(), getCommandBuffer(), 0, {data->screenDescriptorSet});
				Renderer::drawMesh(getCommandBuffer(), pipeline.get(), data->screenMesh.get());
				pipeline->end(getCommandBuffer());
			}
		}
	}

	auto CloudRenderer::beginScene(Scene *scene, const glm::mat4 &projView) -> void
	{
		auto group = scene->getRegistry().group<VolumetricCloud>(entt::get<Light, Transform>);

		auto &      camera = scene->getCamera();
		static auto begin  = Application::getTimer().current();

		for (auto entity : group)
		{
			auto [cloud, light, transform] = group.get<VolumetricCloud, Light, Transform>(entity);

			PipelineInfo info;
			info.shader      = data->cloudShader;
			info.groupCountX = data->computeInputs[0]->getWidth() / 16;
			info.groupCountY = data->computeInputs[0]->getWidth() / 16;
			//data->pipeline   = Pipeline::get(info);

			data->uniformObject.invProj        = glm::inverse(camera.first->getProjectionMatrix());
			data->uniformObject.invView        = camera.second->getWorldMatrix();
			data->uniformObject.lightDirection = glm::vec4(glm::normalize(transform.getWorldOrientation() * maple::FORWARD), 1);
			data->uniformObject.lightColor     = light.lightData.color;
			data->uniformObject.cameraPosition = glm::vec4(camera.second->getWorldPosition(), 1.f);

			data->uniformObject.FOV   = camera.first->getFov();
			data->uniformObject.iTime = Application::getTimer().elapsed(begin, Application::getTimer().current()) / 1000000.f;

			data->uniformObject.earthRadius        = cloud.earthRadius;
			data->uniformObject.sphereInnerRadius  = cloud.sphereInnerRadius;
			data->uniformObject.sphereOuterRadius  = cloud.sphereOuterRadius;
			data->uniformObject.coverageMultiplier = cloud.coverage;
			data->uniformObject.cloudSpeed         = cloud.cloudSpeed;
			data->uniformObject.curliness          = cloud.curliness;
			data->uniformObject.absorption         = cloud.absorption * 0.01;
			data->uniformObject.densityFactor      = cloud.density;
			data->uniformObject.crispiness         = cloud.crispiness;

			data->pipeline = Pipeline::get(info);

			weatherPass->uniformObject.perlinFrequency = cloud.perlinFrequency;
			if (cloud.weathDirty)
			{
				weatherPass->execute(getCommandBuffer());
			}
			break;
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

	auto CloudRenderer::getTexture(CloudsTextures id) -> std::shared_ptr<Texture>
	{
		return data->computeInputs[id];
	}

	auto CloudRenderer::onImGui() -> void
	{
		ImGui::TextUnformatted("Cloud Renderer");
		ImGui::Separator();
		ImGui::Columns(2);
		ImGuiHelper::property("Cloud : Steps", data->uniformObject.steps, 1, 64);
		ImGui::Columns(1);
	}

};        // namespace maple
