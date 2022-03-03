//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "PostProcessRenderer.h"
#include "Engine/Profiler.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/CaptureGraph.h"

#include "RHI/Shader.h"
#include "RHI/DescriptorSet.h"
#include "RHI/Pipeline.h"
#include "RHI/CommandBuffer.h"

#include "Scene/Scene.h"

#include "Others/Randomizer.h"
#include "Math/MathUtils.h"
#include <glm/glm.hpp>
#include "RendererData.h"
#include "Engine/GBuffer.h"

#include <ecs/ecs.h>


namespace maple
{
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
				scale = MathUtils::lerp(0.1f, 1.0f, scale * scale, false);
				ssaoKernels.emplace_back(glm::vec4(sample * scale, 0));
			}
			return ssaoKernels;
		}
	}

	component::SSRData::SSRData()
	{
		ssrShader = Shader::create("shaders/SSR.shader");
		ssrDescriptorSet = DescriptorSet::create({ 0, ssrShader.get() });
	}

	component::SSAOData::SSAOData()
	{
		ssaoShader = Shader::create("shaders/SSAO.shader");
		ssaoBlurShader = Shader::create("shaders/SSAOBlur.shader");

		DescriptorInfo info{};
		ssaoSet.resize(1);
		ssaoBlurSet.resize(1);

		info.shader = ssaoShader.get();
		info.layoutIndex = 0;
		ssaoSet[0] = DescriptorSet::create(info);

		info.shader = ssaoBlurShader.get();
		info.layoutIndex = 0;
		ssaoBlurSet[0] = DescriptorSet::create(info);

		auto ssao = ssaoKernel();
		ssaoSet[0]->setUniformBufferData("UBOSSAOKernel", ssao.data());
	}

	namespace ssao_pass
	{
		using Entity = ecs::Chain
			::Write<component::SSAOData>
			::Read<component::RendererData>
			::Write<capture_graph::component::RenderGraph>
			::To<ecs::Entity>;

		inline auto system(Entity entity, ecs::World world)
		{
			auto [ssaoData, renderData,graph] = entity;

			if (!ssaoData.enable)
				return;

			auto descriptorSet = ssaoData.ssaoSet[0];
			descriptorSet->setTexture("uViewPositionSampler", renderData.gbuffer->getBuffer(GBufferTextures::VIEW_POSITION));
			descriptorSet->setTexture("uViewNormalSampler", renderData.gbuffer->getBuffer(GBufferTextures::VIEW_NORMALS));
			descriptorSet->setTexture("uSsaoNoise", renderData.gbuffer->getSSAONoise());
			descriptorSet->update();

			auto commandBuffer = renderData.commandBuffer;

			PipelineInfo pipeInfo;
			pipeInfo.shader = ssaoData.ssaoShader;
			pipeInfo.polygonMode = PolygonMode::Fill;
			pipeInfo.cullMode = CullMode::None;
			pipeInfo.transparencyEnabled = false;
			pipeInfo.depthBiasEnabled = false;
			pipeInfo.clearTargets = true;
			pipeInfo.depthTest = false;
			pipeInfo.colorTargets[0] = renderData.gbuffer->getBuffer(GBufferTextures::SSAO_SCREEN);
			auto pipeline = Pipeline::get(pipeInfo, ssaoData.ssaoSet, graph);

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
	}

	namespace ssao_blur_pass
	{
		using Entity = ecs::Chain
			::Write<component::SSAOData>
			::Read<component::RendererData>
			::Write<capture_graph::component::RenderGraph>
			::To<ecs::Entity>;

		inline auto system(Entity entity, ecs::World world)
		{
			auto [ssaoData, renderData,graph] = entity;
			if (!ssaoData.enable)
				return;
			auto descriptorSet = ssaoData.ssaoBlurSet[0];
			descriptorSet->setTexture("uSsaoSampler", renderData.gbuffer->getBuffer(GBufferTextures::SSAO_SCREEN));
			descriptorSet->update();

			auto commandBuffer = renderData.commandBuffer;

			PipelineInfo pipeInfo;
			pipeInfo.shader = ssaoData.ssaoBlurShader;
			pipeInfo.polygonMode = PolygonMode::Fill;
			pipeInfo.cullMode = CullMode::None;
			pipeInfo.transparencyEnabled = false;
			pipeInfo.depthBiasEnabled = false;
			pipeInfo.clearTargets = true;
			pipeInfo.depthTest = false;
			pipeInfo.colorTargets[0] = renderData.gbuffer->getBuffer(GBufferTextures::SSAO_BLUR);
			auto pipeline = Pipeline::get(pipeInfo, ssaoData.ssaoBlurSet, graph);

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
	}

	namespace ssr_pass 
	{
		using Entity = ecs::Chain
			::Write<component::SSRData>
			::Read<component::RendererData>
			::Write<capture_graph::component::RenderGraph>
			::To<ecs::Entity>;

		inline auto system(Entity entity, ecs::World world)
		{
			auto [ssrData, render,graph] = entity;
			if (!ssrData.enable)
				return;
			ssrData.ssrDescriptorSet->setTexture("uViewPositionSampler", render.gbuffer->getBuffer(GBufferTextures::VIEW_POSITION));
			ssrData.ssrDescriptorSet->setTexture("uViewNormalSampler", render.gbuffer->getBuffer(GBufferTextures::VIEW_NORMALS));
			ssrData.ssrDescriptorSet->setTexture("uPBRSampler", render.gbuffer->getBuffer(GBufferTextures::PBR));
			ssrData.ssrDescriptorSet->setTexture("uScreenSampler", render.gbuffer->getBuffer(GBufferTextures::SCREEN));
			ssrData.ssrDescriptorSet->update();

			auto commandBuffer = render.commandBuffer;

			PipelineInfo pipeInfo;
			pipeInfo.shader = ssrData.ssrShader;
			pipeInfo.polygonMode = PolygonMode::Fill;
			pipeInfo.cullMode = CullMode::None;
			pipeInfo.transparencyEnabled = false;
			pipeInfo.depthBiasEnabled = false;
			pipeInfo.clearTargets = true;
			pipeInfo.depthTest = false;
			pipeInfo.colorTargets[0] = render.gbuffer->getBuffer(GBufferTextures::SSR_SCREEN);

			auto pipeline = Pipeline::get(pipeInfo, { ssrData.ssrDescriptorSet }, graph);

			if (commandBuffer)
				commandBuffer->bindPipeline(pipeline.get());
			else
				pipeline->bind(commandBuffer);

			Renderer::bindDescriptorSets(pipeline.get(), commandBuffer, 0, { ssrData.ssrDescriptorSet });
			Renderer::drawMesh(commandBuffer, pipeline.get(), render.screenQuad.get());

			if (commandBuffer)
				commandBuffer->unbindPipeline();
			else
				pipeline->end(commandBuffer);
		}
	}

	namespace post_process
	{
		auto registerSSAOPass(ExecuteQueue& begin, ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerGlobalComponent<component::SSAOData>();
			executePoint->registerWithinQueue<ssao_pass::system>(renderer);
			executePoint->registerWithinQueue<ssao_blur_pass::system>(renderer);
		}

		auto registerSSR(ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerGlobalComponent<component::SSRData>();
			executePoint->registerWithinQueue<ssr_pass::system>(renderer);
		}
	};
};        // namespace maple
