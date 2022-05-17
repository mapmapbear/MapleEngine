//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "LPVIndirectLighting.h"
#include "Engine/Renderer/RendererData.h"
#include "Engine/CaptureGraph.h"
#include "Engine/GBuffer.h"
#include "Scene/Component/BoundingBox.h"

#include "RHI/Shader.h"
#include "RHI/Pipeline.h"
#include "RHI/CommandBuffer.h"

#include "LightPropagationVolume.h"

#include <ecs/ecs.h>

namespace maple
{
	namespace lpv_indirect_lighting 
	{
		namespace component 
		{
			struct IndirectLight 
			{
				std::shared_ptr<Shader>                     shader;
				std::vector<std::shared_ptr<DescriptorSet>> descriptorSets;
				IndirectLight() 
				{
					shader = Shader::create("shaders/LPV/IndirectLight.shader");
					descriptorSets.emplace_back(DescriptorSet::create({0,shader.get()}));
				}
			};
		};

		using Entity = ecs::Chain
			::Write<capture_graph::component::RenderGraph>
			::Read<component::IndirectLight>
			::Read<maple::component::RendererData>
			::Read<maple::component::LPVGrid>
			::Read<maple::component::BoundingBoxComponent>
			::To<ecs::Entity>;

		inline auto dispatch(Entity entity, ecs::World world)
		{
			auto [renderGraph, indirectLight, renderData, lpv, aabb] = entity;
			
			if (lpv.lpvAccumulatorR == nullptr) 
				return;

			auto commandBuffer = renderData.computeCommandBuffer;

			indirectLight.descriptorSets[0]->setUniform("UniformBufferObject", "minAABB", glm::value_ptr(aabb.box->min));
			indirectLight.descriptorSets[0]->setUniform("UniformBufferObject", "cellSize", &lpv.cellSize);

			indirectLight.descriptorSets[0]->setTexture("uRAccumulatorLPV", lpv.lpvAccumulatorR);
			indirectLight.descriptorSets[0]->setTexture("uGAccumulatorLPV", lpv.lpvAccumulatorG);
			indirectLight.descriptorSets[0]->setTexture("uBAccumulatorLPV", lpv.lpvAccumulatorB);
			indirectLight.descriptorSets[0]->setTexture("uWorldNormalSampler", renderData.gbuffer->getBuffer(GBufferTextures::NORMALS));
			indirectLight.descriptorSets[0]->setTexture("uWorldPositionSampler", renderData.gbuffer->getBuffer(GBufferTextures::POSITION));
			indirectLight.descriptorSets[0]->setTexture("uIndirectLight", renderData.gbuffer->getBuffer(GBufferTextures::INDIRECT_LIGHTING));
			indirectLight.descriptorSets[0]->update(commandBuffer);

			PipelineInfo pipelineInfo;
			pipelineInfo.shader = indirectLight.shader;
			pipelineInfo.groupCountX = renderData.gbuffer->getWidth() / indirectLight.shader->getLocalSizeX();
			pipelineInfo.groupCountY = renderData.gbuffer->getHeight() / indirectLight.shader->getLocalSizeY();
			auto pipeline = Pipeline::get(pipelineInfo, indirectLight.descriptorSets, renderGraph);
			pipeline->bind(commandBuffer);
			Renderer::bindDescriptorSets(pipeline.get(), commandBuffer, 0, indirectLight.descriptorSets);
			Renderer::dispatch(commandBuffer, pipelineInfo.groupCountX, pipelineInfo.groupCountY, 1);
			pipeline->end(commandBuffer);
		}

		auto registerGlobalComponent(std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerGlobalComponent<component::IndirectLight>();
		}

		auto registerLPVIndirectLight(ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerWithinQueue<lpv_indirect_lighting::dispatch>(renderer);
		}
	}
}
