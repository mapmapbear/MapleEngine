//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "AtmosphereRenderer.h"
#include "RHI/Pipeline.h"
#include "RHI/Shader.h"
#include "RHI/Texture.h"

#include "Engine/Camera.h"
#include "Engine/GBuffer.h"
#include "Engine/Mesh.h"
#include "Engine/Profiler.h"

#include "Scene/Component/Atmosphere.h"
#include "Scene/Component/Light.h"
#include "Scene/Component/Transform.h"
#include "Scene/Scene.h"

#include "Others/Randomizer.h"

#include "ImGui/ImGuiHelpers.h"

#include "Application.h"

namespace maple
{
	struct UniformBufferObject
	{
		glm::mat4 projView;

		glm::vec4 rayleighScattering;        // rayleigh + surfaceRadius
		glm::vec4 mieScattering;             // mieScattering + atmosphereRadius
		glm::vec4 sunDirection;              //sunDirection + sunIntensity
		glm::vec4 centerPoint;
	};

	struct AtmosphereRenderer::RenderData
	{
		std::shared_ptr<Shader>        atmosphereShader;
		std::shared_ptr<DescriptorSet> descriptorSet;
		UniformBufferObject            uniformObject;
		std::shared_ptr<Pipeline>      pipeline;

		std::shared_ptr<Mesh> screenMesh;

		bool renderScreen = false;

		RenderData()
		{
			atmosphereShader = Shader::create("shaders/Atmosphere.shader");
			descriptorSet    = DescriptorSet::create({0, atmosphereShader.get()});
			screenMesh       = Mesh::createQuad(true);
			memset(&uniformObject, 0, sizeof(UniformBufferObject));
		}
	};

	AtmosphereRenderer::AtmosphereRenderer()
	{
	}

	AtmosphereRenderer::~AtmosphereRenderer()
	{
		delete data;
	}

	auto AtmosphereRenderer::init(const std::shared_ptr<GBuffer> &buffer) -> void
	{
		data    = new RenderData();
		gbuffer = buffer;
	}

	auto AtmosphereRenderer::renderScene() -> void
	{
		PROFILE_FUNCTION();
		if (data->pipeline)
		{
			data->descriptorSet->setUniformBufferData("UniformBuffer", &data->uniformObject);
			data->descriptorSet->update();
			data->pipeline->bind(getCommandBuffer());
			Renderer::bindDescriptorSets(data->pipeline.get(), getCommandBuffer(), 0, {data->descriptorSet});
			Renderer::drawMesh(getCommandBuffer(), data->pipeline.get(), data->screenMesh.get());
			data->pipeline->end(getCommandBuffer());
		}
	}

	auto AtmosphereRenderer::beginScene(Scene *scene, const glm::mat4 &projView) -> void
	{
		auto group = scene->getRegistry().group<Atmosphere>(entt::get<Light, Transform>);

		auto &      camera = scene->getCamera();
		static auto begin  = Application::getTimer().current();

		for (auto entity : group)
		{
			auto [atmosphere, light, transform] = group.get<Atmosphere, Light, Transform>(entity);

			const auto &proj          = camera.first->getProjectionMatrix();
			auto        inverseCamerm = camera.second->getWorldMatrixInverse();
			inverseCamerm[3]          = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

			PipelineInfo info;
			info.shader = data->atmosphereShader;
			if (data->renderScreen)
			{
				info.depthTarget = gbuffer->getDepthBuffer();
			}
			info.colorTargets[0]     = gbuffer->getBuffer(data->renderScreen ? GBufferTextures::SCREEN : GBufferTextures::PSEUDO_SKY);
			info.polygonMode         = PolygonMode::Fill;
			info.clearTargets        = false;
			info.transparencyEnabled = false;

			data->uniformObject.projView           = glm::inverse(proj * inverseCamerm);
			data->uniformObject.sunDirection       = {glm::vec3(light.lightData.direction), light.lightData.intensity};
			data->uniformObject.rayleighScattering = {atmosphere.getData().rayleighScattering, atmosphere.getData().surfaceRadius * 1000.f};
			data->uniformObject.mieScattering      = {atmosphere.getData().mieScattering, atmosphere.getData().atmosphereRadius * 1000.f};
			data->uniformObject.centerPoint        = {atmosphere.getData().centerPoint.x * 1000.f, atmosphere.getData().centerPoint.y * 1000.f, atmosphere.getData().centerPoint.z * 1000.f, atmosphere.getData().g};
			data->pipeline                         = Pipeline::get(info);

			break;
		}
	}

	auto AtmosphereRenderer::onImGui() -> void
	{
		ImGui::TextUnformatted("Atmosphere Renderer");
		ImGui::Separator();
		ImGui::Columns(2);
		ImGuiHelper::property("Render Target(Screen or Skybox Buffer)", data->renderScreen);
		ImGui::Columns(1);
	}

};        // namespace maple
