///////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "SkyboxRenderer.h"
#include "RHI/GPUProfile.h"
#include "RHI/Pipeline.h"
#include "RHI/Shader.h"
#include "RHI/Texture.h"

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

#include "Application.h"

#include <glm/gtc/type_ptr.hpp>

namespace maple
{
	namespace
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

	struct SkyboxRenderer::SkyboxData
	{
		struct UniformBufferObject
		{
			glm::mat4 invProj;
			glm::mat4 invView;
			glm::vec4 skyColorBottom;
			glm::vec4 skyColorTop;
			glm::vec4 lightDirection;
			glm::vec4 viewPos;
		};

		std::shared_ptr<Shader>        skyboxShader;
		std::shared_ptr<Pipeline>      pipeline;
		std::shared_ptr<DescriptorSet> descriptorSet;
		std::shared_ptr<Mesh>          skyboxMesh;

		bool pseudoSky = false;

		std::shared_ptr<Shader>        pseudoSkyshader;
		std::shared_ptr<DescriptorSet> pseudoSkydescriptorSet;
		std::shared_ptr<Mesh>          screenMesh;

		UniformBufferObject skyUniformObject;

		std::shared_ptr<Texture> skybox;
		std::shared_ptr<Texture> environmentMap;
		std::shared_ptr<Texture> irradianceMap;

		glm::mat4 projView = glm::mat4(1);

		std::shared_ptr<PrefilterRenderer> prefilterRenderer;

		int32_t cubeMapMode  = 0;
		float   cubeMapLevel = 0;

		SkyboxData()
		{
			pseudoSkyshader        = Shader::create("shaders/PseudoSky.shader");
			pseudoSkydescriptorSet = DescriptorSet::create({0, pseudoSkyshader.get()});
			screenMesh             = Mesh::createQuad(true);
			skyboxShader           = Shader::create("shaders/Skybox.shader");
			descriptorSet          = DescriptorSet::create({0, skyboxShader.get()});
			skyboxMesh             = Mesh::createCube();

			irradianceMap  = TextureCube::create(1);
			environmentMap = irradianceMap;

			prefilterRenderer = std::make_shared<PrefilterRenderer>();
			prefilterRenderer->init();

			memset(&skyUniformObject, 0, sizeof(UniformBufferObject));
		}
	};

	SkyboxRenderer::SkyboxRenderer()
	{
	}

	SkyboxRenderer::~SkyboxRenderer()
	{
		delete skyboxData;
	}

	auto SkyboxRenderer::init(const std::shared_ptr<GBuffer> &buffer) -> void
	{
		skyboxData = new SkyboxData();
		gbuffer    = buffer;
	}

	auto SkyboxRenderer::renderScene() -> void
	{
		PROFILE_FUNCTION();
		GPUProfile("SkyBox Pass");
		if (skyboxData->pseudoSky)
		{
			skyboxData->pseudoSkydescriptorSet->update();
			PipelineInfo info;
			info.shader              = skyboxData->pseudoSkyshader;
			info.colorTargets[0]     = gbuffer->getBuffer(GBufferTextures::PSEUDO_SKY);
			info.polygonMode         = PolygonMode::Fill;
			info.clearTargets        = true;
			info.transparencyEnabled = false;

			auto pipeline = Pipeline::get(info);

			skyboxData->pseudoSkydescriptorSet->setTexture("uPositionSampler", gbuffer->getBuffer(GBufferTextures::POSITION));
			skyboxData->pseudoSkydescriptorSet->update();

			pipeline->bind(getCommandBuffer());
			Renderer::bindDescriptorSets(pipeline.get(), getCommandBuffer(), 0, {skyboxData->pseudoSkydescriptorSet});
			Renderer::drawMesh(getCommandBuffer(), pipeline.get(), skyboxData->screenMesh.get());
			pipeline->end(getCommandBuffer());
		}
		else
		{
			skyboxData->prefilterRenderer->renderScene();

			if (skyboxData->skybox == nullptr)
			{
				return;
			}

			PipelineInfo pipelineInfo{};
			pipelineInfo.shader = skyboxData->skyboxShader;

			pipelineInfo.polygonMode         = PolygonMode::Fill;
			pipelineInfo.cullMode            = CullMode::Front;
			pipelineInfo.transparencyEnabled = false;
			//pipelineInfo.clearTargets        = false;

			pipelineInfo.depthTarget     = gbuffer->getDepthBuffer();
			pipelineInfo.colorTargets[0] = gbuffer->getBuffer(GBufferTextures::SCREEN);

			auto skyboxPipeline = Pipeline::get(pipelineInfo);
			skyboxPipeline->bind(getCommandBuffer());
			if (skyboxData->cubeMapMode == 0)
			{
				skyboxData->descriptorSet->setTexture("uCubeMap", skyboxData->skybox);
			}
			else if (skyboxData->cubeMapMode == 1)
			{
				skyboxData->descriptorSet->setTexture("uCubeMap", skyboxData->environmentMap);
			}
			else if (skyboxData->cubeMapMode == 2)
			{
				skyboxData->descriptorSet->setTexture("uCubeMap", skyboxData->irradianceMap);
			}

			skyboxData->descriptorSet->setUniform("UniformBufferObjectLod", "lodLevel", &skyboxData->cubeMapLevel);
			skyboxData->descriptorSet->update();

			auto &constants = skyboxData->skyboxShader->getPushConstants();
			constants[0].setValue("projView", glm::value_ptr(skyboxData->projView));
			skyboxData->skyboxShader->bindPushConstants(getCommandBuffer(), skyboxPipeline.get());

			Renderer::bindDescriptorSets(skyboxPipeline.get(), getCommandBuffer(), 0, {skyboxData->descriptorSet});
			Renderer::drawMesh(getCommandBuffer(), skyboxPipeline.get(), skyboxData->skyboxMesh.get());
			skyboxPipeline->end(getCommandBuffer());
		}
	}

	auto SkyboxRenderer::beginScene(Scene *scene, const glm::mat4 &projView) -> void
	{
		auto view = scene->getRegistry().view<Environment>();

		auto &camera = scene->getCamera();
		if (view.size() > 0)
		{
			auto &env             = view.get<Environment>(view[0]);
			skyboxData->pseudoSky = env.isPseudoSky();
			if (env.isPseudoSky())
			{
				auto sunLight = scene->getRegistry().group<Light>(entt::get<Transform>);

				skyboxData->skyUniformObject.skyColorBottom = glm::vec4(env.getSkyColorBottom(), 1);
				skyboxData->skyUniformObject.skyColorTop    = glm::vec4(env.getSkyColorTop(), 1);
				skyboxData->skyUniformObject.viewPos        = glm::vec4(camera.second->getWorldPosition(), 1.f);
				skyboxData->skyUniformObject.invView        = camera.second->getWorldMatrix();
				skyboxData->skyUniformObject.invProj        = glm::inverse(camera.first->getProjectionMatrix());

				if (sunLight.size() > 0)
				{
					auto &light                                 = sunLight.get<Light>(sunLight[0]);
					auto &transform                             = sunLight.get<Transform>(sunLight[0]);
					skyboxData->skyUniformObject.lightDirection = glm::vec4(glm::normalize(transform.getWorldOrientation() * maple::FORWARD), 1);
				}

				skyboxData->pseudoSkydescriptorSet->setUniformBufferData("UniformBufferObject", &skyboxData->skyUniformObject);
			}
			else
			{
				skyboxData->prefilterRenderer->beginScene(scene);

				if (skyboxData->skybox != env.getEnvironment())
				{
					skyboxData->environmentMap = env.getPrefilteredEnvironment();
					skyboxData->irradianceMap  = env.getIrradianceMap();
					skyboxData->skybox         = env.getEnvironment();
				}
				const auto &proj          = camera.first->getProjectionMatrix();
				auto        inverseCamerm = camera.second->getWorldMatrixInverse();
				inverseCamerm[3]          = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
				skyboxData->projView      = proj * inverseCamerm;
			}
		}
		else
		{
			if (skyboxData->skybox)
			{
				skyboxData->environmentMap = nullptr;
				skyboxData->skybox         = nullptr;
				skyboxData->irradianceMap  = nullptr;
			}
		}
	}

	auto SkyboxRenderer::onResize(uint32_t width, uint32_t height) -> void
	{
	}

	auto SkyboxRenderer::onImGui() -> void
	{
		ImGui::Separator();
		ImGui::TextUnformatted("SkyboxRenderer");
		ImGui::Separator();
		ImGui::Columns(2);

		ImGui::TextUnformatted("CubeMap LodLevel");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		ImGui::DragFloat("##CubeMap LodLevel", &skyboxData->cubeMapLevel, 0.5f, 0.0f, 4.0f);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::Columns(1);

		if (ImGui::BeginMenu(cubeMapModeToString(skyboxData->cubeMapMode).c_str()))
		{
			constexpr int32_t numModes = 3;

			for (int32_t i = 0; i < numModes; i++)
			{
				if (ImGui::MenuItem(cubeMapModeToString(i).c_str(), "", skyboxData->cubeMapMode == i, true))
				{
					skyboxData->cubeMapMode = i;
				}
			}
			ImGui::EndMenu();
		}
	}

};        // namespace maple
