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

	namespace
	{
		inline auto ssaoKernel() -> const std::vector<glm::vec4>
		{
			constexpr int32_t      SSAO_KERNEL_SIZE = 64;
			std::vector<glm::vec4> ssaoKernels;
			for (auto i = 0; i < SSAO_KERNEL_SIZE; ++i)
			{
				glm::vec3 sample(
				    Randomizer::random() * 2.0 - 1.0,
				    Randomizer::random() * 2.0 - 1.0,
				    Randomizer::random());
				sample = glm::normalize(sample);
				sample *= Randomizer::random();
				float scale = float(i) / float(SSAO_KERNEL_SIZE);
				scale       = MathUtils::lerp(0.1f, 1.0f, scale * scale, false);
				ssaoKernels.emplace_back(glm::vec4(sample * scale, 0));
			}
			return ssaoKernels;
		}
	}        // namespace

	namespace ssao_pass
	{
		using Entity = ecs::Registry ::Modify<component::SSAOData>::Fetch<component::RendererData>::Modify<capture_graph::component::RenderGraph>::Fetch<component::CameraView>::To<ecs::Entity>;

		inline auto system(Entity entity, ecs::World world)
		{
			auto [ssaoData, renderData, graph, camera] = entity;

			if (!ssaoData.enable)
				return;

			auto descriptorSet = ssaoData.ssaoSet[0];
			descriptorSet->setTexture("uViewPositionSampler", renderData.gbuffer->getBuffer(GBufferTextures::VIEW_POSITION));
			descriptorSet->setTexture("uViewNormalSampler", renderData.gbuffer->getBuffer(GBufferTextures::VIEW_NORMALS));
			descriptorSet->setTexture("uSsaoNoise", renderData.gbuffer->getSSAONoise());
			descriptorSet->setUniform("UBO", "ssaoRadius", &ssaoData.ssaoRadius);
			descriptorSet->setUniform("UBO", "projection", &camera.proj);
			descriptorSet->update(renderData.commandBuffer);

			auto commandBuffer = renderData.commandBuffer;

			PipelineInfo pipeInfo;
			pipeInfo.pipelineName        = "SSAORenderer";
			pipeInfo.shader              = ssaoData.ssaoShader;
			pipeInfo.polygonMode         = PolygonMode::Fill;
			pipeInfo.cullMode            = CullMode::None;
			pipeInfo.transparencyEnabled = false;
			pipeInfo.depthBiasEnabled    = false;
			pipeInfo.clearTargets        = true;
			pipeInfo.depthTest           = false;
			pipeInfo.colorTargets[0]     = renderData.gbuffer->getBuffer(GBufferTextures::SSAO_SCREEN);
			auto pipeline                = Pipeline::get(pipeInfo, ssaoData.ssaoSet, graph);

			if (commandBuffer)
				commandBuffer->bindPipeline(pipeline.get());
			else
				pipeline->bind(commandBuffer);

			Renderer::bindDescriptorSets(pipeline.get(), commandBuffer, 0, ssaoData.ssaoSet);
			Renderer::drawMesh(commandBuffer, pipeline.get(), renderData.screenQuad.get());

			if (commandBuffer)
				commandBuffer->unbindPipeline();
			else
				pipeline->end(commandBuffer);
		}
	}        // namespace ssao_pass

	namespace ssao_blur_pass
	{
		using Entity = ecs::Registry ::Modify<component::SSAOData>::Fetch<component::RendererData>::Modify<capture_graph::component::RenderGraph>::To<ecs::Entity>;

		inline auto system(Entity entity, ecs::World world)
		{
			auto [ssaoData, renderData, graph] = entity;
			if (!ssaoData.enable)
				return;
			auto descriptorSet = ssaoData.ssaoBlurSet[0];
			descriptorSet->setTexture("uSsaoSampler", renderData.gbuffer->getBuffer(GBufferTextures::SSAO_SCREEN));
			descriptorSet->update(renderData.commandBuffer);

			auto commandBuffer = renderData.commandBuffer;

			PipelineInfo pipeInfo;
			pipeInfo.pipelineName        = "SSAOBlurRenderer";
			pipeInfo.shader              = ssaoData.ssaoBlurShader;
			pipeInfo.polygonMode         = PolygonMode::Fill;
			pipeInfo.cullMode            = CullMode::None;
			pipeInfo.transparencyEnabled = false;
			pipeInfo.depthBiasEnabled    = false;
			pipeInfo.clearTargets        = true;
			pipeInfo.depthTest           = false;
			pipeInfo.colorTargets[0]     = renderData.gbuffer->getBuffer(GBufferTextures::SSAO_BLUR);
			auto pipeline                = Pipeline::get(pipeInfo, ssaoData.ssaoBlurSet, graph);

			if (commandBuffer)
				commandBuffer->bindPipeline(pipeline.get());
			else
				pipeline->bind(commandBuffer);

			Renderer::bindDescriptorSets(pipeline.get(), commandBuffer, 0, ssaoData.ssaoBlurSet);
			Renderer::drawMesh(commandBuffer, pipeline.get(), renderData.screenQuad.get());

			if (commandBuffer)
				commandBuffer->unbindPipeline();
			else
				pipeline->end(commandBuffer);
		}
	}        // namespace ssao_blur_pass

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
		auto registerSSAOPass(ExecuteQueue &begin, ExecuteQueue &renderer, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerGlobalComponent<component::SSAOData>([](auto &ssao) {
				ssao.ssaoShader     = Shader::create("shaders/SSAO.shader");
				ssao.ssaoBlurShader = Shader::create("shaders/SSAOBlur.shader");

				DescriptorInfo info{};
				ssao.ssaoSet.resize(1);
				ssao.ssaoBlurSet.resize(1);

				info.shader      = ssao.ssaoShader.get();
				info.layoutIndex = 0;
				ssao.ssaoSet[0]  = DescriptorSet::create(info);

				info.shader         = ssao.ssaoBlurShader.get();
				info.layoutIndex    = 0;
				ssao.ssaoBlurSet[0] = DescriptorSet::create(info);

				auto ssaoKernel2 = ssaoKernel();
				ssao.ssaoSet[0]->setUniformBufferData("UBOSSAOKernel", ssaoKernel2.data());
			});
			executePoint->registerWithinQueue<ssao_pass::system>(renderer);
			executePoint->registerWithinQueue<ssao_blur_pass::system>(renderer);
		}

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
