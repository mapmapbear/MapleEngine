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

				skyboxData.pseudoSky = envData.isPseudoSky();
				if (envData.isPseudoSky())
				{
					skyboxData.skyUniformObject.skyColorBottom = glm::vec4(envData.getSkyColorBottom(), 1);
					skyboxData.skyUniformObject.skyColorTop = glm::vec4(envData.getSkyColorTop(), 1);
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

					if (skyboxData.skybox != envData.getEnvironment())
					{
						skyboxData.environmentMap = envData.getPrefilteredEnvironment();
						skyboxData.irradianceMap = envData.getIrradianceMap();
						skyboxData.skybox = envData.getEnvironment();
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

		inline auto onRender(Entity entity, ecs::World world)
		{
			auto [skyboxData, cameraView, graph] = entity;

			auto& renderData = world.getComponent<component::RendererData>(entity);

			GPUProfile("SkyBox Pass");
			if (skyboxData.pseudoSky)
			{
				skyboxData.pseudoSkydescriptorSet->update();
				PipelineInfo info;
				info.shader = skyboxData.pseudoSkyshader;
				info.colorTargets[0] = renderData.gbuffer->getBuffer(GBufferTextures::PSEUDO_SKY);
				info.polygonMode = PolygonMode::Fill;
				info.clearTargets = true;
				info.transparencyEnabled = false;

				auto pipeline = Pipeline::get(info, { skyboxData.pseudoSkydescriptorSet },graph);

				skyboxData.pseudoSkydescriptorSet->setTexture("uPositionSampler", renderData.gbuffer->getBuffer(GBufferTextures::POSITION));
				skyboxData.pseudoSkydescriptorSet->update();

				pipeline->bind(renderData.commandBuffer);
				Renderer::bindDescriptorSets(pipeline.get(), renderData.commandBuffer, 0, { skyboxData.pseudoSkydescriptorSet });
				Renderer::drawMesh(renderData.commandBuffer, pipeline.get(), skyboxData.screenMesh.get());
				pipeline->end(renderData.commandBuffer);
			}
			else
			{
				skyboxData.prefilterRenderer->renderScene(graph);

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

				pipelineInfo.depthTarget = renderData.gbuffer->getDepthBuffer();
				pipelineInfo.colorTargets[0] = renderData.gbuffer->getBuffer(GBufferTextures::SCREEN);

				auto skyboxPipeline = Pipeline::get(pipelineInfo, {skyboxData.descriptorSet}, graph);
				skyboxPipeline->bind(renderData.commandBuffer);
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
				skyboxData.descriptorSet->update();

				auto& constants = skyboxData.skyboxShader->getPushConstants();
				constants[0].setValue("projView", glm::value_ptr(skyboxData.projView));
				skyboxData.skyboxShader->bindPushConstants(renderData.commandBuffer, skyboxPipeline.get());

				Renderer::bindDescriptorSets(skyboxPipeline.get(), renderData.commandBuffer, 0, { skyboxData.descriptorSet });
				Renderer::drawMesh(renderData.commandBuffer, skyboxPipeline.get(), skyboxData.skyboxMesh.get());
				skyboxPipeline->end(renderData.commandBuffer);
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
