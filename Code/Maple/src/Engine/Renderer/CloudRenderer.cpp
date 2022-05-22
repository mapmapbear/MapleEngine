//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "CloudRenderer.h"
#include "RHI/Pipeline.h"
#include "RHI/Shader.h"
#include "RHI/Texture.h"

#include "Engine/Camera.h"
#include "Engine/GBuffer.h"
#include "Engine/Mesh.h"
#include "Engine/Profiler.h"
#include "Engine/CaptureGraph.h"

#include "Scene/Component/Light.h"
#include "Scene/Component/Transform.h"
#include "Scene/Component/VolumetricCloud.h"
#include "Scene/Scene.h"

#include "Others/Randomizer.h"

#include "ImGui/ImGuiHelpers.h"

#include "RendererData.h"

#include "Application.h"

#include <ecs/ecs.h>

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

	namespace component 
	{
		struct CloudRenderData
		{
			std::shared_ptr<Shader>                 cloudShader;
			std::shared_ptr<Pipeline>               pipeline;
			std::shared_ptr<DescriptorSet>          descriptorSet;
			std::vector<std::shared_ptr<Texture2D>> computeInputs;

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
				float   frames;
				int32_t enablePowder;
			} uniformObject;

			std::shared_ptr<Shader>        screenCloudShader;
			std::shared_ptr<DescriptorSet> screenDescriptorSet;

			CloudRenderData()
			{
				cloudShader = Shader::create("shaders/Cloud.shader");
				descriptorSet = DescriptorSet::create({ 0, cloudShader.get() });
				memset(&uniformObject, 0, sizeof(UniformBufferObject));
				uniformObject.steps = 64;

				screenCloudShader = Shader::create("shaders/CloudScreen.shader");
				screenDescriptorSet = DescriptorSet::create({ 0, screenCloudShader.get() });
			}
		};

		struct WeatherPass
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
				float     perlinScale = 100.0;
				int       perlinOctaves = 4;
			} uniformObject;

			bool generatedNoise = false;

			WeatherPass()
			{
				shader = Shader::create("shaders/Weather.shader");
				descriptorSet = DescriptorSet::create({ 0, shader.get() });
				weather = Texture2D::create();
				weather->buildTexture(TextureFormat::RGBA32, 1024, 1024, false, true, false, false, true, 2);
				uniformObject.seed = { Randomizer::random(), Randomizer::random(), Randomizer::random() };

				perlinWorley = Shader::create("shaders/PerlinWorley.shader");
				perlin3D = Texture3D::create( 128, 128, 128, {}, {false,false,true} );
				perlinWorleySet = DescriptorSet::create({ 0, perlinWorley.get() });
				perlinWorleySet->setTexture("outVolTex", perlin3D);

				worleyShader = Shader::create("shaders/Worley.shader");
				worley3D = Texture3D::create(32, 32, 32, {}, {false,false,true} );
				worleySet = DescriptorSet::create({ 0, worleyShader.get() });
				worleySet->setTexture("outVolTex", worley3D);
			}

			inline auto executePerlin3D(CommandBuffer* cmd, capture_graph::component::RenderGraph& graph)
			{
				PipelineInfo info;
				info.shader = perlinWorley;
				auto pipeline = Pipeline::get(info, { perlinWorleySet },graph);
				perlinWorleySet->update(cmd);
				pipeline->bind(cmd);
				Renderer::bindDescriptorSets(pipeline.get(), cmd, 0, { perlinWorleySet });
				Renderer::dispatch(cmd, 128 / 4, 128 / 4, 128 / 4);
				pipeline->end(cmd);
				perlin3D->generateMipmaps(cmd);
			}

			inline auto executeWorley3D(CommandBuffer* cmd, capture_graph::component::RenderGraph& graph)
			{
				PipelineInfo info;
				info.shader = worleyShader;
				auto pipeline = Pipeline::get(info, { worleySet },graph);
				worleySet->update(cmd);
				pipeline->bind(cmd);
				Renderer::bindDescriptorSets(pipeline.get(), cmd, 0, { worleySet });
				Renderer::dispatch(cmd, 32 / 4, 32 / 4, 32 / 4);
				pipeline->end(cmd);
				worley3D->generateMipmaps(cmd);
			}

			inline auto execute(CommandBuffer* cmd, capture_graph::component::RenderGraph& graph)
			{
				descriptorSet->setUniformBufferData("UniformBufferObject", &uniformObject);
				descriptorSet->setTexture("outWeatherTex", weather);
				descriptorSet->update(cmd);

				PipelineInfo info;
				info.shader = shader;
				auto pipeline = Pipeline::get(info, { descriptorSet },graph);
				pipeline->bind(cmd);
				Renderer::bindDescriptorSets(pipeline.get(), cmd, 0, { descriptorSet });
				Renderer::dispatch(cmd, 1024 / 8, 1024 / 8, 1);
				pipeline->end(cmd);
			}
		};
	}
	
	namespace begin_scene 
	{
		using Entity = ecs::Chain
			::Write<component::CloudRenderData>
			::Read<component::CameraView>
			::Read<component::RendererData>
			::Write<component::WeatherPass>
			::Write<Timer>
			::Read<component::WindowSize>
			::Write<capture_graph::component::RenderGraph>
			::To<ecs::Entity>;

		using Query = ecs::Chain
			::Read<component::VolumetricCloud>
			::Read<component::Light>
			::Read<component::Transform>
			::To<ecs::Query>;

		inline auto system(Entity entity, Query query, ecs::World world)
		{
			auto [data, camera, render, weather, timer, winSize,graph] = entity;
			static auto begin = timer.current();

			if (!weather.generatedNoise)
			{
				weather.executePerlin3D(render.computeCommandBuffer,graph);
				weather.executeWorley3D(render.computeCommandBuffer,graph);
				weather.generatedNoise = true;
			}

			if (query.empty() || winSize.height == 0 || winSize.width == 0)
				return;

			if (data.computeInputs.empty()) 
			{
				for (auto i = 0; i < data.cloudShader->getDescriptorInfo(0).size(); i++)
				{
					data.computeInputs.emplace_back(Texture2D::create());
				}
			}

			auto descs = data.cloudShader->getDescriptorInfo(0);
			for (auto& desc : descs)
			{
				auto& binding = data.computeInputs[desc.binding];
				if(binding->getWidth() != winSize.width || binding->getHeight() != winSize.height)
					binding->buildTexture(TextureFormat::RGBA32, winSize.width, winSize.height, false, true, false, false, true, desc.accessFlag);
			}
	

			for (auto entity : query)
			{
				auto [cloud, light, transform] = query.convert(entity);

				
				data.uniformObject.invProj = glm::inverse(camera.proj);
				data.uniformObject.invView = camera.cameraTransform->getWorldMatrix();
				data.uniformObject.lightDirection = light.lightData.direction;
					//glm::vec4(glm::normalize(transform.getWorldOrientation() * maple::FORWARD), 1);
				data.uniformObject.lightColor = light.lightData.color;
				data.uniformObject.cameraPosition = glm::vec4(camera.cameraTransform->getWorldPosition(), 1.f);

				data.uniformObject.FOV = camera.fov;
				data.uniformObject.iTime = timer.elapsed(begin, timer.current()) / 1000000.f;

				data.uniformObject.earthRadius = cloud.earthRadius;
				data.uniformObject.sphereInnerRadius = cloud.sphereInnerRadius;
				data.uniformObject.sphereOuterRadius = cloud.sphereOuterRadius;
				data.uniformObject.coverageMultiplier = cloud.coverage;
				data.uniformObject.cloudSpeed = cloud.cloudSpeed;
				data.uniformObject.curliness = cloud.curliness;
				data.uniformObject.absorption = cloud.absorption * 0.01;
				data.uniformObject.densityFactor = cloud.density;
				data.uniformObject.crispiness = cloud.crispiness;
				data.uniformObject.enablePowder = cloud.enablePowder ? 1 : 0;

				PipelineInfo info;
				info.shader = data.cloudShader;
				info.groupCountX = data.computeInputs[0]->getWidth() / 16;
				info.groupCountY = data.computeInputs[0]->getWidth() / 16;



				data.descriptorSet->setTexture("fragColor", data.computeInputs[0]);
				data.descriptorSet->setTexture("bloom", data.computeInputs[1]);
				data.descriptorSet->setTexture("alphaness", data.computeInputs[2]);
				data.descriptorSet->setTexture("cloudDistance", data.computeInputs[3]);
				data.descriptorSet->setTexture("uDepthSampler", render.gbuffer->getDepthBuffer());
				data.descriptorSet->setTexture("uSky", render.gbuffer->getBuffer(GBufferTextures::PSEUDO_SKY));
				data.descriptorSet->setTexture("uWeatherTex", weather.weather);
				data.descriptorSet->setTexture("uCloud", weather.perlin3D);
				data.descriptorSet->setTexture("uWorley32", weather.worley3D);

				data.pipeline = Pipeline::get(info, { data.descriptorSet }, graph);

				weather.uniformObject.perlinFrequency = cloud.perlinFrequency;

				if (cloud.weathDirty)
				{
					weather.execute(render.commandBuffer, graph);
				}
				break;
			}
		}
	}

	namespace on_render
	{
		using Entity = ecs::Chain
			::Write<component::CloudRenderData>
			::Read<component::CameraView>
			::Read<component::RendererData>
			::Read<component::WeatherPass>
			::Write<capture_graph::component::RenderGraph>
			::To<ecs::Entity>;

		inline auto system(Entity entity, ecs::World world)
		{
			auto [data, camera, render,weather,graph] = entity;

			if (data.pipeline)
			{
				{
					data.uniformObject.frames++;
					data.descriptorSet->setUniformBufferData("UniformBufferObject", &data.uniformObject);
					data.descriptorSet->update(render.commandBuffer);
					data.pipeline->bind(render.commandBuffer);
					Renderer::bindDescriptorSets(data.pipeline.get(), render.commandBuffer, 0, { data.descriptorSet });
					Renderer::dispatch(render.commandBuffer,
						render.gbuffer->getWidth() / data.cloudShader->getLocalSizeX(),
						render.gbuffer->getHeight() / data.cloudShader->getLocalSizeY(),
						1);
					data.pipeline->end(render.commandBuffer);
				}

				{
					data.screenDescriptorSet->update(render.commandBuffer);
					PipelineInfo info;
					info.shader = data.screenCloudShader;
					info.colorTargets[0] = render.gbuffer->getBuffer(GBufferTextures::SCREEN);
					info.polygonMode = PolygonMode::Fill;
					info.clearTargets = false;
					info.transparencyEnabled = false;
					info.depthTarget = render.gbuffer->getDepthBuffer();

					auto pipeline = Pipeline::get(info, { data.screenDescriptorSet }, graph);

					data.screenDescriptorSet->setTexture("uCloudSampler", data.computeInputs[0]);
					data.screenDescriptorSet->update(render.commandBuffer);

					pipeline->bind(render.commandBuffer);
					Renderer::bindDescriptorSets(pipeline.get(), render.commandBuffer, 0, { data.screenDescriptorSet });
					Renderer::drawMesh(render.commandBuffer, pipeline.get(), render.screenQuad.get());
					pipeline->end(render.commandBuffer);
				}
			}
		}
	}

	namespace cloud_renderer
	{
		auto registerCloudRenderer(ExecuteQueue& begin, ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerGlobalComponent<Timer>();
			executePoint->registerGlobalComponent<component::CloudRenderData>();
			executePoint->registerGlobalComponent<component::WeatherPass>();

			executePoint->registerWithinQueue<begin_scene::system>(begin);
			executePoint->registerWithinQueue<on_render::system>(renderer);
		}
	}
};        // namespace maple
