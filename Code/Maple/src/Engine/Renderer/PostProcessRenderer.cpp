//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "PostProcessRenderer.h"
#include "Engine/CaptureGraph.h"
#include "Engine/Profiler.h"
#include "Engine/Renderer/Renderer.h"

#include "RHI/CommandBuffer.h"
#include "RHI/DescriptorSet.h"
#include "RHI/Pipeline.h"
#include "RHI/Shader.h"

#include "Scene/Scene.h"

#include "Engine/GBuffer.h"
#include "Math/MathUtils.h"
#include "Others/Randomizer.h"
#include "RendererData.h"
#include <glm/glm.hpp>

#include <ecs/ecs.h>

namespace maple
{
	namespace component
	{
		struct ComputeBloom
		{
			std::shared_ptr<DescriptorSet> bloomDescriptorSet;
			std::shared_ptr<Shader>        bloomShader;
		};
	}        // namespace component
     // namespace


	namespace ssr_pass
	{
		using Entity = ecs::Registry ::Modify<component::SSRData>::Fetch<component::RendererData>::Modify<capture_graph::component::RenderGraph>::To<ecs::Entity>;

		inline auto system(Entity entity, ecs::World world)
		{
			auto [ssrData, render, graph] = entity;
			if (!ssrData.enable)
				return;
			ssrData.ssrDescriptorSet->setTexture("uViewPositionSampler", render.gbuffer->getBuffer(GBufferTextures::VIEW_POSITION));
			ssrData.ssrDescriptorSet->setTexture("uViewNormalSampler", render.gbuffer->getBuffer(GBufferTextures::VIEW_NORMALS));
			ssrData.ssrDescriptorSet->setTexture("uPBRSampler", render.gbuffer->getBuffer(GBufferTextures::PBR));
			ssrData.ssrDescriptorSet->setTexture("uScreenSampler", render.gbuffer->getBuffer(GBufferTextures::SCREEN));
			auto commandBuffer = render.commandBuffer;
			ssrData.ssrDescriptorSet->update(commandBuffer);

			PipelineInfo pipeInfo;
			pipeInfo.pipelineName        = "SSRRenderer";
			pipeInfo.shader              = ssrData.ssrShader;
			pipeInfo.polygonMode         = PolygonMode::Fill;
			pipeInfo.cullMode            = CullMode::None;
			pipeInfo.transparencyEnabled = false;
			pipeInfo.depthBiasEnabled    = false;
			pipeInfo.clearTargets        = true;
			pipeInfo.depthTest           = false;
			pipeInfo.colorTargets[0]     = render.gbuffer->getBuffer(GBufferTextures::SSR_SCREEN);

			auto pipeline = Pipeline::get(pipeInfo, {ssrData.ssrDescriptorSet}, graph);

			if (commandBuffer)
				commandBuffer->bindPipeline(pipeline.get());
			else
				pipeline->bind(commandBuffer);

			Renderer::bindDescriptorSets(pipeline.get(), commandBuffer, 0, {ssrData.ssrDescriptorSet});
			Renderer::drawMesh(commandBuffer, pipeline.get(), render.screenQuad.get());

			if (commandBuffer)
				commandBuffer->unbindPipeline();
			else
				pipeline->end(commandBuffer);
		}
	}        // namespace ssr_pass

	namespace bloom_pass
	{
		using Entity2 = ecs::Registry ::Fetch<component::BloomData>::Modify<component::ComputeBloom>::Fetch<component::RendererData>::Modify<capture_graph::component::RenderGraph>::Fetch<component::WindowSize>::To<ecs::Entity>;

		inline auto computeBloom(Entity2 entity, ecs::World world)
		{
			auto [bloom, bloomData, render, graph, winSize] = entity;
			if (!bloom.enable)
				return;

			auto commandBuffer = render.commandBuffer;

			bloomData.bloomDescriptorSet->setTexture("uScreenSampler", render.gbuffer->getBuffer(GBufferTextures::SCREEN));
			bloomData.bloomDescriptorSet->update(commandBuffer);

			PipelineInfo pipeInfo;
			pipeInfo.pipelineName        = "BloomRenderer";
			pipeInfo.shader              = bloomData.bloomShader;
			pipeInfo.polygonMode         = PolygonMode::Fill;
			pipeInfo.cullMode            = CullMode::None;
			pipeInfo.transparencyEnabled = false;
			pipeInfo.depthBiasEnabled    = false;
			pipeInfo.clearTargets        = true;
			pipeInfo.depthTest           = false;
			pipeInfo.colorTargets[0]     = render.gbuffer->getBuffer(GBufferTextures::BLOOM_SCREEN);

			auto pipeline = Pipeline::get(pipeInfo, {bloomData.bloomDescriptorSet}, graph);

			if (commandBuffer)
				commandBuffer->bindPipeline(pipeline.get());
			else
				pipeline->bind(commandBuffer);

			Renderer::bindDescriptorSets(pipeline.get(), commandBuffer, 0, {bloomData.bloomDescriptorSet});
			Renderer::drawMesh(commandBuffer, pipeline.get(), render.screenQuad.get());

			if (commandBuffer)
				commandBuffer->unbindPipeline();
			else
				pipeline->end(commandBuffer);
		}

		using Entity = ecs::Registry ::Modify<component::BloomData>::Fetch<component::RendererData>::Modify<capture_graph::component::RenderGraph>::Fetch<component::WindowSize>::To<ecs::Entity>;

		inline auto gaussBlur(Entity entity, ecs::World world)
		{
			auto [bloomData, render, graph, winSize] = entity;
			if (!bloomData.enable)
				return;

			for (auto i = 0; i < 2; i++)
			{
				//v -> h
				bloomData.bloomDescriptorSet->setTexture("samplerColor",
				                                         i == 0 ?
                                                             render.gbuffer->getBuffer(GBufferTextures::BLOOM_SCREEN) :
                                                             render.gbuffer->getBuffer(GBufferTextures::BLOOM_BLUR));

				int32_t dir = 1 - i;

				auto commandBuffer = render.commandBuffer;

				bloomData.bloomDescriptorSet->setUniform("UBO", "dir", &dir);
				bloomData.bloomDescriptorSet->setUniform("UBO", "blurScale", &bloomData.blurScale);
				bloomData.bloomDescriptorSet->setUniform("UBO", "blurStrength", &bloomData.blurStrength);
				bloomData.bloomDescriptorSet->update(commandBuffer);

				PipelineInfo pipeInfo;
				pipeInfo.pipelineName        = "BloomRenderer";
				pipeInfo.shader              = bloomData.bloomShader;
				pipeInfo.polygonMode         = PolygonMode::Fill;
				pipeInfo.cullMode            = CullMode::None;
				pipeInfo.transparencyEnabled = false;
				pipeInfo.depthBiasEnabled    = false;
				pipeInfo.clearTargets        = false;
				pipeInfo.depthTest           = false;
				pipeInfo.colorTargets[0]     = render.gbuffer->getBuffer(i == 0 ? GBufferTextures::BLOOM_BLUR : GBufferTextures::BLOOM_SCREEN);

				auto pipeline = Pipeline::get(pipeInfo, {bloomData.bloomDescriptorSet}, graph);

				if (commandBuffer)
					commandBuffer->bindPipeline(pipeline.get());
				else
					pipeline->bind(commandBuffer);

				Renderer::bindDescriptorSets(pipeline.get(), commandBuffer, 0, {bloomData.bloomDescriptorSet});
				Renderer::drawMesh(commandBuffer, pipeline.get(), render.screenQuad.get());

				if (commandBuffer)
					commandBuffer->unbindPipeline();
				else
					pipeline->end(commandBuffer);
			}
		}
	}        // namespace bloom_pass

	namespace post_process
	{
		auto registerSSR(ExecuteQueue &renderer, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerGlobalComponent<component::SSRData>([](auto &ssr) {
				ssr.ssrShader        = Shader::create("shaders/SSR.shader");
				ssr.ssrDescriptorSet = DescriptorSet::create({0, ssr.ssrShader.get()});
			});
			executePoint->registerWithinQueue<ssr_pass::system>(renderer);
		}

		auto registerBloom(ExecuteQueue &renderer, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerGlobalComponent<component::BloomData>([](auto &bloom) {
				bloom.bloomShader        = Shader::create("shaders/GaussBlur.shader");
				bloom.bloomDescriptorSet = DescriptorSet::create({0, bloom.bloomShader.get()});
			});

			executePoint->registerGlobalComponent<component::ComputeBloom>([](auto &bloom) {
				bloom.bloomShader        = Shader::create("shaders/ComputeBloom.shader");
				bloom.bloomDescriptorSet = DescriptorSet::create({0, bloom.bloomShader.get()});
			});

			executePoint->registerWithinQueue<bloom_pass::computeBloom>(renderer);
			executePoint->registerWithinQueue<bloom_pass::gaussBlur>(renderer);
		}
	};        // namespace post_process
};            // namespace maple
