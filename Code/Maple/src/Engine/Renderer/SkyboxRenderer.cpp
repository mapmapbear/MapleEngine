///////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "SkyboxRenderer.h"
#include "RHI/GPUProfile.h"
#include "RHI/Pipeline.h"
#include "RHI/Shader.h"
#include "RHI/Texture.h"

#include "Engine/CaptureGraph.h"
#include "Engine/Camera.h"
#include "Engine/GBuffer.h"
#include "Engine/Mesh.h"
#include "Engine/Profiler.h"
#include "PrefilterRenderer.h"

#include "Scene/Component/Light.h"
#include "Scene/Component/Transform.h"
#include "Scene/Component/VolumetricCloud.h"
#include "Scene/Component/Environment.h"
#include "Scene/Scene.h"

#include "Others/Randomizer.h"

#include "ImGui/ImGuiHelpers.h"

#include "RendererData.h"

#include "Application.h"

#include <glm/gtc/type_ptr.hpp>

#include <ecs/ecs.h>

namespace maple
{
	namespace component
	{
		inline auto cubeMapModeToString(int32_t mode) -> const std::string
		{
			switch (mode)
			{
				case 0:
					return "CubeMap";
				case 1:
					return "Prefilter";
				case 2:
					return "Irradiance";
			}
			return "";
		}
	}        // namespace

	namespace skybox_pass
	{
		using Entity = ecs::Chain
			::Write<component::SkyboxData>
			::Read<component::CameraView>
			::Write<capture_graph::component::RenderGraph>
			::To<ecs::Entity>;

		using Query = ecs::Chain
			::Write<component::Environment>
			::To<ecs::Query>;

		using SunLightQuery = ecs::Chain
			::Read<component::Light>
			::Read<component::Transform>
			::To<ecs::Query>;

		inline auto beginScene(Entity entity, Query query, SunLightQuery sunLightQuery, ecs::World world)
		{
			auto [skyboxData,cameraView,graph] = entity;

			if (!query.empty())
			{
				auto entityHandle = *query.begin();
				auto& envData = query.getComponent<component::Environment>(entityHandle);

				skyboxData.pseudoSky = envData.pseudoSky;
				if (envData.pseudoSky)
				{
					skyboxData.skyUniformObject.skyColorBottom = glm::vec4(envData.skyColorBottom, 1);
					skyboxData.skyUniformObject.skyColorTop = glm::vec4(envData.skyColorTop, 1);
					skyboxData.skyUniformObject.viewPos = glm::vec4(cameraView.cameraTransform->getWorldPosition(), 1.f);
					skyboxData.skyUniformObject.invView = cameraView.cameraTransform->getWorldMatrix();
					skyboxData.skyUniformObject.invProj = glm::inverse(cameraView.proj);

					if (!sunLightQuery.empty())
					{
						auto [light, transform] = sunLightQuery.convert(*sunLightQuery.begin());
						skyboxData.skyUniformObject.lightDirection = light.lightData.direction;
					}
					skyboxData.pseudoSkydescriptorSet->setUniformBufferData("UniformBufferObject", &skyboxData.skyUniformObject);
				}
				else
				{
					skyboxData.prefilterRenderer->beginScene(envData);

					if (skyboxData.skybox != envData.environment)
					{
						skyboxData.environmentMap = envData.prefilteredEnvironment;
						skyboxData.irradianceMap = envData.irradianceMap;
						skyboxData.skybox = envData.environment;
					}

					auto inverseCamerm = cameraView.view;
					inverseCamerm[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
					skyboxData.projView = cameraView.proj * inverseCamerm;
				}
			}
			else
			{
				if (skyboxData.skybox)
				{
					skyboxData.environmentMap = nullptr;
					skyboxData.skybox = nullptr;
					skyboxData.irradianceMap = nullptr;
				}
			}
		}

		inline auto onRender(Entity entity, const component::RendererData& rendererData, ecs::World world)
		{
			auto [skyboxData, cameraView, graph] = entity;


			GPUProfile("SkyBox Pass");
			if (skyboxData.pseudoSky)
			{
				skyboxData.pseudoSkydescriptorSet->update(rendererData.commandBuffer);
				PipelineInfo info;
				info.shader = skyboxData.pseudoSkyshader;
				info.colorTargets[0] = rendererData.gbuffer->getBuffer(GBufferTextures::PSEUDO_SKY);
				info.polygonMode = PolygonMode::Fill;
				info.clearTargets = true;
				info.transparencyEnabled = false;

				auto pipeline = Pipeline::get(info, { skyboxData.pseudoSkydescriptorSet },graph);

				skyboxData.pseudoSkydescriptorSet->setTexture("uPositionSampler", rendererData.gbuffer->getBuffer(GBufferTextures::POSITION));
				skyboxData.pseudoSkydescriptorSet->update(rendererData.commandBuffer);

				pipeline->bind(rendererData.commandBuffer);
				Renderer::bindDescriptorSets(pipeline.get(), rendererData.commandBuffer, 0, { skyboxData.pseudoSkydescriptorSet });
				Renderer::drawMesh(rendererData.commandBuffer, pipeline.get(), skyboxData.screenMesh.get());
				pipeline->end(rendererData.commandBuffer);
			}
			else
			{
				skyboxData.prefilterRenderer->renderScene(rendererData.commandBuffer,graph);

				if (skyboxData.skybox == nullptr)
				{
					return;
				}

				PipelineInfo pipelineInfo{};
				pipelineInfo.shader = skyboxData.skyboxShader;

				pipelineInfo.polygonMode = PolygonMode::Fill;
				pipelineInfo.cullMode = CullMode::Front;
				pipelineInfo.transparencyEnabled = false;
				//pipelineInfo.clearTargets        = false;

				pipelineInfo.depthTarget = rendererData.gbuffer->getDepthBuffer();
				pipelineInfo.colorTargets[0] = rendererData.gbuffer->getBuffer(GBufferTextures::SCREEN);

				auto skyboxPipeline = Pipeline::get(pipelineInfo, {skyboxData.descriptorSet}, graph);
				skyboxPipeline->bind(rendererData.commandBuffer);
				if (skyboxData.cubeMapMode == 0)
				{
					skyboxData.descriptorSet->setTexture("uCubeMap", skyboxData.skybox);
				}
				else if (skyboxData.cubeMapMode == 1)
				{
					skyboxData.descriptorSet->setTexture("uCubeMap", skyboxData.environmentMap);
				}
				else if (skyboxData.cubeMapMode == 2)
				{
					skyboxData.descriptorSet->setTexture("uCubeMap", skyboxData.irradianceMap);
				}

				skyboxData.descriptorSet->setUniform("UniformBufferObjectLod", "lodLevel", &skyboxData.cubeMapLevel);
				skyboxData.descriptorSet->update(rendererData.commandBuffer);

				auto& constants = skyboxData.skyboxShader->getPushConstants();
				constants[0].setValue("projView", glm::value_ptr(skyboxData.projView));
				skyboxData.skyboxShader->bindPushConstants(rendererData.commandBuffer, skyboxPipeline.get());

				Renderer::bindDescriptorSets(skyboxPipeline.get(), rendererData.commandBuffer, 0, { skyboxData.descriptorSet });
				Renderer::drawMesh(rendererData.commandBuffer, skyboxPipeline.get(), skyboxData.skyboxMesh.get());
				skyboxPipeline->end(rendererData.commandBuffer);
			}
		}
	}


	namespace skybox_renderer
	{
		auto registerSkyboxRenderer(ExecuteQueue& begin, ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerGlobalComponent<component::SkyboxData>([](auto & skybox) {
				skybox.pseudoSkyshader = Shader::create("shaders/PseudoSky.shader");
				skybox.pseudoSkydescriptorSet = DescriptorSet::create({ 0,skybox.pseudoSkyshader.get() });
				skybox.screenMesh = Mesh::createQuad(true);
				skybox.skyboxShader = Shader::create("shaders/Skybox.shader");
				skybox.descriptorSet = DescriptorSet::create({ 0, skybox.skyboxShader.get() });
				skybox.skyboxMesh = Mesh::createCube();

				skybox.irradianceMap = TextureCube::create(1);
				skybox.environmentMap = skybox.irradianceMap;

				skybox.irradianceMap->setName("uIrradianceSampler");
				skybox.irradianceMap->setName("uEnvironmentSampler");

				skybox.prefilterRenderer = std::make_shared<PrefilterRenderer>();
				skybox.prefilterRenderer->init();
				memset(&skybox.skyUniformObject, 0, sizeof(component::SkyboxData::UniformBufferObject));
			});
			executePoint->registerWithinQueue<skybox_pass::beginScene>(begin);
			executePoint->registerWithinQueue<skybox_pass::onRender>(renderer);
		}
	};
};        // namespace maple
