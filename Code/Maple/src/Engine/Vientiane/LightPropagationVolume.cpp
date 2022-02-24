//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "LightPropagationVolume.h"
#include "ReflectiveShadowMap.h"
#include "Engine/Renderer/RendererData.h"
#include "Engine/Mesh.h"
#include "RHI/IndexBuffer.h"
#include "RHI/VertexBuffer.h"
#include "RHI/Shader.h"
#include "RHI/Pipeline.h"
#include "RHI/DescriptorSet.h"
#include <ecs/ecs.h>

namespace maple
{
	namespace component
	{
		struct InjectLightData
		{
			std::shared_ptr<Shader> shader;
			std::vector<std::shared_ptr<DescriptorSet>> descriptors;

			InjectLightData()
			{

			}
		};
	};

	namespace light_propagation_volume
	{
		namespace inject_light_pass 
		{
			using Entity = ecs::Chain
				::Read<component::LPVData>
				::Read<component::ReflectiveShadowData>
				::Read<component::RendererData>
				::Read<component::InjectLightData>
				::To<ecs::Entity>;
				
			inline auto beginScene(Entity entity, ecs::World world)
			{
			}

			inline auto render(Entity entity, ecs::World world)
			{
				auto [lpv, rsm, rendererData,injectionLight] = entity;

				lpv.lpvGridR->clear();
				lpv.lpvGridG->clear();
				lpv.lpvGridB->clear();

				PipelineInfo pipelineInfo;
				pipelineInfo.shader = injectionLight.shader;
				pipelineInfo.groupCountX = component::ReflectiveShadowData::SHADOW_SIZE / injectionLight.shader->getLocalSizeX();
				pipelineInfo.groupCountY = component::ReflectiveShadowData::SHADOW_SIZE / injectionLight.shader->getLocalSizeY();

				auto pipeline = Pipeline::get(pipelineInfo);
				pipeline->bind(rendererData.commandBuffer);
				Renderer::bindDescriptorSets(pipeline.get(), rendererData.commandBuffer, 0, injectionLight.descriptors);
				Renderer::dispatch(rendererData.commandBuffer,pipelineInfo.groupCountX,pipelineInfo.groupCountY,1);
				pipeline->end(rendererData.commandBuffer);
			}
		};

		namespace inject_geometry_pass
		{
			using Entity = ecs::Chain
				::Read<component::LPVData>
				::To<ecs::Entity>;

			inline auto beginScene(Entity entity, ecs::World world)
			{

			}

			inline auto render(Entity entity, ecs::World world)
			{

			}
		};

		namespace propagation_pass
		{
			using Entity = ecs::Chain
				::Read<component::LPVData>
				::To<ecs::Entity>;

			inline auto beginScene(Entity entity, ecs::World world)
			{

			}

			inline auto render(Entity entity, ecs::World world)
			{

			}
		};


		auto registerLPV(ExecuteQueue& begin, ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerGlobalComponent<component::LPVData>();
			executePoint->registerWithinQueue<inject_light_pass::beginScene>(begin);
			executePoint->registerWithinQueue<inject_light_pass::render>(renderer);
			executePoint->registerWithinQueue<inject_geometry_pass::beginScene>(begin);
			executePoint->registerWithinQueue<inject_geometry_pass::render>(renderer);
			executePoint->registerWithinQueue<propagation_pass::beginScene>(begin);
			executePoint->registerWithinQueue<propagation_pass::render>(renderer);
		}
	};
};        // namespace maple
