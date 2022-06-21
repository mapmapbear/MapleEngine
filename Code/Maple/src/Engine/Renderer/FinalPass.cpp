//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "FinalPass.h"

#include "Engine/CaptureGraph.h"

#include "Engine/Renderer/PostProcessRenderer.h"
#include "Engine/Renderer/Renderer.h"

#include "RHI/CommandBuffer.h"
#include "RHI/DescriptorSet.h"
#include "RHI/Pipeline.h"
#include "RHI/Shader.h"

#include "Scene/Scene.h"

#include "Engine/GBuffer.h"
#include "Math/MathUtils.h"
#include "RendererData.h"
#include <glm/glm.hpp>

#include <ecs/ecs.h>

namespace maple
{
	namespace final_screen_pass
	{
		using Entity = ecs::Registry ::OptinalFetch<component::BloomData>::Fetch<component::FinalPass>::Fetch<component::RendererData>::Modify<capture_graph::component::RenderGraph>::To<ecs::Entity>;

		inline auto system(Entity entity, ecs::World world)
		{
			auto [finalData, renderData, graph] = entity;
			float gamma                         = 2.2;

			finalData.finalDescriptorSet->setUniform("UniformBuffer", "gamma", &gamma);
			finalData.finalDescriptorSet->setUniform("UniformBuffer", "toneMapIndex", &finalData.toneMapIndex);
			finalData.finalDescriptorSet->setUniform("UniformBuffer", "exposure", &finalData.exposure);
			auto ssaoEnable    = 0;
			auto reflectEnable = 0;
			auto cloudEnable   = false;        // envData->cloud ? 1 : 0;

			int32_t bloomEnable = 0;

			if (entity.hasComponent<component::BloomData>())
			{
				bloomEnable = entity.getComponent<component::BloomData>().enable;
			}

			finalData.finalDescriptorSet->setUniform("UniformBuffer", "bloomEnable", &bloomEnable);
			finalData.finalDescriptorSet->setUniform("UniformBuffer", "ssaoEnable", &ssaoEnable);
			finalData.finalDescriptorSet->setUniform("UniformBuffer", "reflectEnable", &reflectEnable);
			finalData.finalDescriptorSet->setUniform("UniformBuffer", "cloudEnable", &cloudEnable);

			finalData.finalDescriptorSet->setTexture("uScreenSampler", renderData.gbuffer->getBuffer(GBufferTextures::SCREEN));

			//if (reflectEnable)
			{
				finalData.finalDescriptorSet->setTexture("uReflectionSampler", renderData.gbuffer->getBuffer(GBufferTextures::SSR_SCREEN));
			}

			//if (bloomEnable)
			{
				finalData.finalDescriptorSet->setTexture("uBloomSampler", renderData.gbuffer->getBuffer(GBufferTextures::BLOOM_SCREEN));
			}

			finalData.finalDescriptorSet->update(renderData.commandBuffer);

			PipelineInfo pipelineDesc{};
			pipelineDesc.shader = finalData.finalShader;

			pipelineDesc.polygonMode         = PolygonMode::Fill;
			pipelineDesc.cullMode            = CullMode::None;
			pipelineDesc.transparencyEnabled = false;

			if (finalData.renderTarget)
				pipelineDesc.colorTargets[0] = finalData.renderTarget;
			else
				pipelineDesc.swapChainTarget = true;

			auto pipeline = Pipeline::get(pipelineDesc, {finalData.finalDescriptorSet}, graph);
			pipeline->bind(renderData.commandBuffer);

			Renderer::bindDescriptorSets(pipeline.get(), renderData.commandBuffer, 0, {finalData.finalDescriptorSet});
			Renderer::drawMesh(renderData.commandBuffer, pipeline.get(), renderData.screenQuad.get());

			pipeline->end(renderData.commandBuffer);
		}
	}        // namespace final_screen_pass

	namespace final_screen_pass
	{
		auto registerFinalPass(ExecuteQueue &renderer, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerGlobalComponent<component::FinalPass>([](component::FinalPass &data) {
				data.finalShader = Shader::create("shaders/ScreenPass.shader");
				DescriptorInfo descriptorInfo{};
				descriptorInfo.layoutIndex = 0;
				descriptorInfo.shader      = data.finalShader.get();
				data.finalDescriptorSet    = DescriptorSet::create(descriptorInfo);
			});
			executePoint->registerWithinQueue<final_screen_pass::system>(renderer);
		}
	};        // namespace final_screen_pass
};            // namespace maple
