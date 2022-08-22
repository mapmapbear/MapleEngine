//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "SSAORenderer.h"

#include "RHI/CommandBuffer.h"
#include "RHI/DescriptorSet.h"
#include "RHI/Pipeline.h"
#include "RHI/Shader.h"

#include "Engine/GBuffer.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/RendererData.h"
#include "Engine/CaptureGraph.h"

#include "Math/MathUtils.h"
#include "Others/Randomizer.h"
#include <glm/glm.hpp>

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
				scale       = MathUtils::lerp(0.1f, 1.0f, scale * scale, false);
				ssaoKernels.emplace_back(glm::vec4(sample * scale, 0));
			}
			return ssaoKernels;
		}
	}        // namespace

	namespace ssao
	{
		namespace ssao_pass
		{
			// clang-format off
			using Entity = ecs::Registry 
				::Modify<component::SSAOData>
				::Fetch<maple::component::RendererData>
				::Modify<capture_graph::component::RenderGraph>
				::Fetch<maple::component::CameraView>
				::To<ecs::Entity>;
			// clang-format on

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
			// clang-format off
			using Entity = ecs::Registry 
				::Modify<component::SSAOData>
				::Fetch<maple::component::RendererData>
				::Modify<capture_graph::component::RenderGraph>
				::To<ecs::Entity>;
			// clang-format on

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
	}        // namespace ssao
}        // namespace maple