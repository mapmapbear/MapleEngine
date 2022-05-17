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
#include "Engine/CaptureGraph.h"


#include "Scene/Component/Atmosphere.h"
#include "Scene/Component/Light.h"
#include "Scene/Component/Transform.h"
#include "Scene/Scene.h"

#include "Others/Randomizer.h"

#include "ImGui/ImGuiHelpers.h"

#include "Application.h"

#include "RendererData.h"
#include <ecs/ecs.h>

namespace maple
{
	namespace component
	{
		struct AtmosphereData
		{
			std::shared_ptr<Shader>        atmosphereShader;
			std::shared_ptr<DescriptorSet> descriptorSet;
			struct UniformBufferObject
			{
				glm::mat4 projView;
				glm::vec4 rayleighScattering;        // rayleigh + surfaceRadius
				glm::vec4 mieScattering;             // mieScattering + atmosphereRadius
				glm::vec4 sunDirection;              //sunDirection + sunIntensity
				glm::vec4 centerPoint;
			}uniformObject;
			std::shared_ptr<Pipeline>      pipeline;

			AtmosphereData()
			{
				atmosphereShader = Shader::create("shaders/Atmosphere.shader");
				descriptorSet = DescriptorSet::create({ 0, atmosphereShader.get() });
				memset(&uniformObject, 0, sizeof(UniformBufferObject));
			}
		};
	}

	namespace atmosphere_pass
	{
		namespace begin_scene
		{
			using Entity = ecs::Chain
				::Write<component::AtmosphereData>
				::Read<component::RendererData>
				::Read<component::CameraView>
				::Write<capture_graph::component::RenderGraph>
				::To<ecs::Entity>;

			using Query = ecs::Chain
				::Read<component::Atmosphere>
				::Read<component::Light>
				::Read<component::Transform>
				::To<ecs::Query>;

			inline auto system(Entity entity, Query query, ecs::World world)
			{
				auto [data,render, camera,graph] = entity;

				for (auto entity : query)
				{
					auto [atmosphere, light, transform] = query.convert(entity);

					auto inverseCamerm = camera.view;
					inverseCamerm[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

					PipelineInfo info;
					info.shader = data.atmosphereShader;
					if (atmosphere.renderToScreen)
					{
						info.depthTarget = render.gbuffer->getDepthBuffer();
					}
					info.colorTargets[0] = render.gbuffer->getBuffer(atmosphere.renderToScreen ? GBufferTextures::SCREEN : GBufferTextures::PSEUDO_SKY);
					info.polygonMode = PolygonMode::Fill;
					info.clearTargets = !atmosphere.renderToScreen;
					info.transparencyEnabled = false;

					data.uniformObject.projView = glm::inverse(camera.proj * inverseCamerm);
					data.uniformObject.sunDirection = { glm::vec3(-light.lightData.direction), light.lightData.intensity };
					data.uniformObject.rayleighScattering = { atmosphere.rayleighScattering, atmosphere.surfaceRadius * 1000.f };
					data.uniformObject.mieScattering = { atmosphere.mieScattering, atmosphere.atmosphereRadius * 1000.f };
					data.uniformObject.centerPoint = { atmosphere.centerPoint.x * 1000.f, atmosphere.centerPoint.y * 1000.f, atmosphere.centerPoint.z * 1000.f, atmosphere.g };
					data.pipeline = Pipeline::get(info, {data.descriptorSet}, graph);

					break;
				}
			}
		}

		namespace on_render
		{
			using Entity = ecs::Chain
				::Write<component::AtmosphereData>
				::Read<component::RendererData>
				::To<ecs::Entity>;

			inline auto system(Entity entity,  ecs::World world)
			{
				auto [data, render] = entity;

				if (data.pipeline) 
				{
					data.descriptorSet->setUniformBufferData("UniformBuffer", &data.uniformObject);
					data.descriptorSet->update(render.commandBuffer);
					data.pipeline->bind(render.commandBuffer);
					Renderer::bindDescriptorSets(data.pipeline.get(), render.commandBuffer, 0, { data.descriptorSet });
					Renderer::drawMesh(render.commandBuffer, data.pipeline.get(), render.screenQuad.get());
					data.pipeline->end(render.commandBuffer);
				}
			}
		}
	}
	
	namespace atmosphere_pass
	{
		auto registerAtmosphere(ExecuteQueue& begin, ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerGlobalComponent<component::AtmosphereData>();

			executePoint->registerWithinQueue<begin_scene::system>(begin);
			executePoint->registerWithinQueue<on_render::system>(renderer);
		}
	}
};        // namespace maple
