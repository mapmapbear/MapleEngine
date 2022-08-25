//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "DDGIVisualization.h"
#include "DDGIRenderer.h"
#include "Engine/GBuffer.h"
#include "Engine/Mesh.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/RendererData.h"
#include "RHI/DescriptorSet.h"
#include "RHI/Pipeline.h"

namespace maple
{
	namespace ddgi
	{
		namespace component
		{
			struct VisualizationPipeline
			{
				Shader::Ptr   shader;
				Pipeline::Ptr pipeline;
				Mesh::Ptr     sphere;

				std::vector<DescriptorSet::Ptr> descriptorSets;
			};
		}        // namespace component

		namespace delegates
		{
			inline auto initVisualization(ddgi::component::DDGIVisualization &viusal, Entity entity, ecs::World world)
			{
				auto &pipeline   = entity.addComponent<component::VisualizationPipeline>();
				auto &renderData = world.getComponent<maple::component::RendererData>();

				pipeline.shader = Shader::create("shaders/DDGI/ProbeVisualization.shader");
				PipelineInfo info;
				info.pipelineName    = "Probe-Visualization";
				info.polygonMode     = PolygonMode::Fill;
				info.blendMode       = BlendMode::SrcAlphaOneMinusSrcAlpha;
				info.clearTargets    = false;
				info.swapChainTarget = false;
				info.depthTarget     = renderData.gbuffer->getDepthBuffer();
				info.colorTargets[0] = renderData.gbuffer->getBuffer(GBufferTextures::SCREEN);
				pipeline.pipeline    = Pipeline::get(info);
				pipeline.sphere      = Mesh::createSphere();

				pipeline.descriptorSets[0] = DescriptorSet::create({0, pipeline.shader.get()});
				pipeline.descriptorSets[1] = DescriptorSet::create({1, pipeline.shader.get()});
			}
		}        // namespace delegates

		namespace debug_ddgi
		{
			// clang-format off
			using Entity = ecs::Registry 
				::Fetch<component::DDGIVisualization>
				::Modify<component::VisualizationPipeline>
				::Fetch<component::DDGIUniform>
				::Fetch<component::DDGIPipeline>
				::To<ecs::Entity>;
			// clang-format on

			inline auto system(Entity                          entity,
			                   maple::component::RendererData &renderData,
			                   maple::component::CameraView &  cameraView,
			                   ecs::World                      world)
			{
				auto [visual, pipeline, uniform, ddgipipe] = entity;
				if (visual.enable)
				{
					pipeline.descriptorSets[0]->setUniform("UniformBufferObject", "viewProj", &cameraView.projView);
					pipeline.descriptorSets[1]->setUniform("DDGIUBO", "ddgi", &uniform);
					pipeline.descriptorSets[1]->setTexture("uIrradiance", ddgipipe.currentIrrdance);
					pipeline.descriptorSets[1]->setTexture("uDepth", ddgipipe.currentDepth);

					pipeline.descriptorSets[0]->update(renderData.commandBuffer);
					pipeline.descriptorSets[1]->update(renderData.commandBuffer);

					auto probeCount = uniform.probeCounts.x * uniform.probeCounts.y * uniform.probeCounts.z;

					pipeline.pipeline->bind(renderData.commandBuffer);

					if (auto push = pipeline.shader->getPushConstant(0))
					{
						push->setData(&visual.scale);
					}

					Renderer::bindDescriptorSets(pipeline.pipeline.get(), renderData.commandBuffer, 0, pipeline.descriptorSets);

					pipeline.shader->bindPushConstants(renderData.commandBuffer, pipeline.pipeline.get());

					pipeline.pipeline->drawIndexed(
					    renderData.commandBuffer,
					    pipeline.sphere->getIndexBuffer()->getCount(), probeCount, 0, 0, 0);

					pipeline.pipeline->end(renderData.commandBuffer);
				}
			}
		}        // namespace debug_ddgi

		auto registerDDGIVisualization(ExecuteQueue &begin, ExecuteQueue &render, std::shared_ptr<ExecutePoint> point) -> void
		{
			point->onConstruct<ddgi::component::DDGIVisualization, delegates::initVisualization>();
			point->registerWithinQueue<debug_ddgi::system>(render);
		}
	}        // namespace ddgi
}        // namespace maple