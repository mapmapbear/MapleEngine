//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "FinalPass.h"
#include "Engine/Renderer/Renderer.h"

#include "RHI/Shader.h"
#include "RHI/DescriptorSet.h"
#include "RHI/Pipeline.h"
#include "RHI/CommandBuffer.h"

#include "Scene/Scene.h"

#include "Math/MathUtils.h"
#include <glm/glm.hpp>
#include "RendererData.h"
#include "Engine/GBuffer.h"

#include <ecs/ecs.h>


namespace maple
{

	namespace final_screen_pass
	{
		using Entity = ecs::Chain
			::Read<component::FinalPass>
			::Read<component::RendererData>
			::To<ecs::Entity>;

		inline auto system(Entity entity, ecs::World world)
		{
			auto [finalData, renderData] = entity;
			float gamma = 2.2;
			int32_t toneMapIndex = 7;

			finalData.finalDescriptorSet->setUniform("UniformBuffer", "gamma", &gamma);
			finalData.finalDescriptorSet->setUniform("UniformBuffer", "toneMapIndex", &toneMapIndex);
			auto ssaoEnable = 0;
			auto reflectEnable = 0;
			auto cloudEnable = false;// envData->cloud ? 1 : 0;

			finalData.finalDescriptorSet->setUniform("UniformBuffer", "ssaoEnable", &ssaoEnable);
			finalData.finalDescriptorSet->setUniform("UniformBuffer", "reflectEnable", &reflectEnable);
			finalData.finalDescriptorSet->setUniform("UniformBuffer", "cloudEnable", &cloudEnable);

			finalData.finalDescriptorSet->setTexture("uScreenSampler", renderData.gbuffer->getBuffer(GBufferTextures::SCREEN));
			finalData.finalDescriptorSet->setTexture("uReflectionSampler", renderData.gbuffer->getBuffer(GBufferTextures::SSR_SCREEN));

			finalData.finalDescriptorSet->update();

			PipelineInfo pipelineDesc{};
			pipelineDesc.shader = finalData.finalShader;

			pipelineDesc.polygonMode = PolygonMode::Fill;
			pipelineDesc.cullMode = CullMode::Back;
			pipelineDesc.transparencyEnabled = false;

			if (finalData.renderTarget)
				pipelineDesc.colorTargets[0] = finalData.renderTarget;
			else
				pipelineDesc.swapChainTarget = true;

			auto pipeline = Pipeline::get(pipelineDesc);
			pipeline->bind(renderData.commandBuffer);

			Renderer::bindDescriptorSets(pipeline.get(), renderData.commandBuffer, 0, { finalData.finalDescriptorSet });
			Renderer::drawMesh(renderData.commandBuffer, pipeline.get(), renderData.screenQuad.get());

			pipeline->end(renderData.commandBuffer);
		}
	}

	namespace final_screen_pass
	{
		auto registerFinalPass(ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerGlobalComponent<component::FinalPass>();
			executePoint->registerWithinQueue<final_screen_pass::system>(renderer);
		}
	};

	component::FinalPass::FinalPass()
	{
		finalShader = Shader::create("shaders/ScreenPass.shader");
		DescriptorInfo descriptorInfo{};
		descriptorInfo.layoutIndex = 0;
		descriptorInfo.shader = finalShader.get();
		finalDescriptorSet = DescriptorSet::create(descriptorInfo);
	}

};        // namespace maple
