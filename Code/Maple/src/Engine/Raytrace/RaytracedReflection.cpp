//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "RaytracedReflection.h"
#include "Engine/DDGI/DDGIRenderer.h"
#include "Engine/GBuffer.h"
#include "Engine/Noise/BlueNoise.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/RendererData.h"

#include "Scene/Component/Transform.h"
#include "Scene/System/BindlessModule.h"

#include "RHI/AccelerationStructure.h"
#include "RHI/DescriptorPool.h"
#include "RHI/GraphicsContext.h"
#include "RHI/Pipeline.h"
#include "RHI/RenderDevice.h"
#include "RHI/StorageBuffer.h"

#include <glm/gtc/type_ptr.hpp>

namespace maple
{
	namespace
	{
		constexpr uint32_t TEMPORAL_ACCUMULATION_NUM_THREADS_X = 8;
		constexpr uint32_t TEMPORAL_ACCUMULATION_NUM_THREADS_Y = 8;
	}        // namespace

	namespace raytraced_reflection
	{
		namespace component
		{
			struct ReflectionPipeline
			{
				Shader::Ptr   traceShader;
				Pipeline::Ptr tracePipeline;

				DescriptorSet::Ptr gbufferSet;        //6 - GBuffer
				DescriptorSet::Ptr noiseSet;          //7 - Noise
				DescriptorSet::Ptr outSet;            //8 - Out...

				Texture2D::Ptr outColor;

				struct PushConstants
				{
					float     bias                = 0.5f;
					float     trim                = 0.8f;
					float     intensity           = 0.5f;
					float     roughDDGIIntensity  = 0.5f;
					uint32_t  numLights           = 0;
					uint32_t  numFrames           = 0;
					uint32_t  sampleGI            = 1;
					uint32_t  approximateWithDDGI = 1;
					glm::vec4 cameraPosition;
				} pushConsts;

				int32_t pingPong = 0;
			};

			struct TemporalAccumulator
			{
				//Temporal - Accumulation
				Texture2D::Ptr outputs[2];        //Reflection Previous Re-projection
				Texture2D::Ptr currentMoments[2];

				DescriptorSet::Ptr writeDescriptorSet;        //Reflection Ray Trace Write
				DescriptorSet::Ptr readDescriptorSet;         //Reflection Ray Trace Read

				StorageBuffer::Ptr denoiseDispatchBuffer;
				StorageBuffer::Ptr denoiseTileCoordsBuffer;        // indirect?

				StorageBuffer::Ptr copyTileCoordsBuffer;
				StorageBuffer::Ptr copyDispatchBuffer;        // indirect?

				Shader::Ptr   resetArgsShader;
				Pipeline::Ptr resetPipeline;

				DescriptorSet::Ptr indirectDescriptorSet;

				/*
				* 0 -> Moment/History
				* 1 -> GBuffer
				* 2 -> PrevGBuffer
				* 3 -> Args
				*/
				std::vector<DescriptorSet::Ptr> descriptorSets;
				Shader::Ptr                     reprojectionShader;
				Pipeline::Ptr                   reprojectionPipeline;

				struct PushConstants
				{
					float alpha        = 0.01f;
					float momentsAlpha = 0.2f;
				} pushConsts;
			};

			struct AtrousFiler
			{
				struct PushConstants
				{
					int   radius        = 1;
					int   stepSize      = 0;
					float phiVisibility = 10.0f;
					float phiNormal     = 32.f;
					float sigmaDepth    = 1.f;
					float power         = 1.2f;
					float near_         = 0.1;
					float far_          = 10000.f;
				} pushConsts;

				int32_t iterations = 4;

				Shader::Ptr   atrousFilerShader;
				Pipeline::Ptr atrousFilerPipeline;

				Shader::Ptr   copyTilesShader;
				Pipeline::Ptr copyTilesPipeline;

				DescriptorSet::Ptr copyWriteDescriptorSet[2];
				DescriptorSet::Ptr copyReadDescriptorSet[2];

				DescriptorSet::Ptr copyTilesSet[2];

				DescriptorSet::Ptr gBufferSet;
				DescriptorSet::Ptr inputSet;
				DescriptorSet::Ptr argsSet;

				Texture2D::Ptr atrousFilter[2];        //A-Trous Filter
			};
		}        // namespace component

		namespace update
		{
			// clang-format off
			using Entity = ecs::Registry
				::Modify<raytraced_reflection::component::ReflectionPipeline>
				::To<ecs::Entity>;

			using Group = ecs::Registry
				::Fetch<ddgi::component::DDGIPipeline>
				::To<ecs::Group>;
			// clang-format on

			inline auto system(Entity                                         entity,
			                   Group                                          group,
			                   const maple::component::WindowSize &           winSize,
			                   const maple::component::RendererData &         rendererData,
			                   const maple::component::CameraView &           cameraView,
			                   const global::component::RaytracingDescriptor &descriptor)
			{
				if (!group.empty())
				{
					auto [pipeline] = entity;

					pipeline.pushConsts.numLights = descriptor.numLights;

					pipeline.pushConsts.cameraPosition = {cameraView.cameraTransform->getWorldPosition(), 1.f};

					pipeline.gbufferSet->setTexture("uPositionSampler", rendererData.gbuffer->getBuffer(GBufferTextures::POSITION));
					pipeline.gbufferSet->setTexture("uNormalSampler", rendererData.gbuffer->getBuffer(GBufferTextures::NORMALS));
					pipeline.gbufferSet->setTexture("uDepthSampler", rendererData.gbuffer->getDepthBuffer());
					pipeline.gbufferSet->setTexture("uPBRSampler", rendererData.gbuffer->getBuffer(GBufferTextures::PBR));

					for (auto &pushConsts : pipeline.traceShader->getPushConstants())
					{
						pushConsts.setData(&pipeline.pushConsts);
					}
				}
			}
		}        // namespace update

		namespace render
		{
			// clang-format off
			using Entity = ecs::Registry
				::Modify<raytraced_reflection::component::ReflectionPipeline>
				::Modify<raytraced_reflection::component::RaytracedReflection>
				::To<ecs::Entity>;

			using Group = ecs::Registry
				::Fetch<ddgi::component::DDGIPipeline>
				::To<ecs::Group>;
			// clang-format on

			inline auto system(Entity                                         entity,
			                   Group                                          group,
			                   const maple::component::WindowSize &           winSize,
			                   const maple::component::RendererData &         rendererData,
			                   const maple::component::CameraView &           cameraView,
			                   const global::component::RaytracingDescriptor &descriptor)
			{
				if (!group.empty())
				{
					auto [pipeline, reflection] = entity;
					auto [ddgiPipeline]         = group.convert(*group.begin());

					pipeline.gbufferSet->update(rendererData.commandBuffer);
					pipeline.noiseSet->update(rendererData.commandBuffer);
					pipeline.outSet->update(rendererData.commandBuffer);

					pipeline.tracePipeline->bind(rendererData.commandBuffer);
					pipeline.traceShader->bindPushConstants(rendererData.commandBuffer, pipeline.tracePipeline.get());

					Renderer::bindDescriptorSets(pipeline.tracePipeline.get(), rendererData.commandBuffer, 0,
					                             {descriptor.sceneDescriptor,
					                              descriptor.vboDescriptor,
					                              descriptor.iboDescriptor,
					                              descriptor.materialDescriptor,
					                              descriptor.textureDescriptor,
					                              ddgiPipeline.ddgiCommon,
					                              pipeline.gbufferSet,
					                              pipeline.noiseSet,
					                              pipeline.outSet});

					pipeline.tracePipeline->traceRays(rendererData.commandBuffer, winSize.width, winSize.height, 1);
					reflection.output = pipeline.outColor;
				}
			}
		}        // namespace render

		namespace denoise
		{
			// clang-format off
			using Entity = ecs::Registry
				::Modify<raytraced_reflection::component::TemporalAccumulator>
				::To<ecs::Entity>;
			// clang-format on

			inline auto resetArgs(component::TemporalAccumulator &acc, const maple::component::RendererData &renderData)
			{
				acc.resetPipeline->bufferBarrier(renderData.commandBuffer, {acc.denoiseDispatchBuffer, acc.denoiseTileCoordsBuffer, acc.copyDispatchBuffer, acc.copyTileCoordsBuffer}, false);
				acc.resetPipeline->bind(renderData.commandBuffer);
				Renderer::bindDescriptorSets(acc.resetPipeline.get(), renderData.commandBuffer, 0, {acc.indirectDescriptorSet});
				Renderer::dispatch(renderData.commandBuffer, 1, 1, 1);
				acc.resetPipeline->end(renderData.commandBuffer);
			}

			inline auto accumulation(component::TemporalAccumulator &      accumulator,
			                         component::ReflectionPipeline &      pipeline,
			                         const maple::component::RendererData &renderData,
			                         const maple::component::WindowSize &  winSize,
			                         const maple::component::CameraView &  cameraView)
			{
				accumulator.descriptorSets[0]->setTexture("outColor", accumulator.outputs[0]);
				accumulator.descriptorSets[0]->setTexture("moment", accumulator.currentMoments[pipeline.pingPong]);
				accumulator.descriptorSets[0]->setTexture("uHistoryOutput", accumulator.outputs[1]);        //prev
				accumulator.descriptorSets[0]->setTexture("uHistoryMoments", accumulator.currentMoments[1 - pipeline.pingPong]);

				accumulator.descriptorSets[0]->setTexture("uInput", pipeline.outColor);        //noised reflection
				accumulator.descriptorSets[0]->setUniform("UniformBufferObject", "viewProjInv", glm::value_ptr(glm::inverse(cameraView.projView)));
				accumulator.descriptorSets[0]->setUniform("UniformBufferObject", "viewProjInvOld", glm::value_ptr(glm::inverse(cameraView.projViewOld)));

				accumulator.descriptorSets[1]->setTexture("uPositionSampler", renderData.gbuffer->getBuffer(GBufferTextures::POSITION));
				accumulator.descriptorSets[1]->setTexture("uNormalSampler", renderData.gbuffer->getBuffer(GBufferTextures::NORMALS));
				accumulator.descriptorSets[1]->setTexture("uVelocitySampler", renderData.gbuffer->getBuffer(GBufferTextures::VELOCITY));
				accumulator.descriptorSets[1]->setTexture("uDepthSampler", renderData.gbuffer->getDepthBuffer());

				accumulator.descriptorSets[2]->setTexture("uPrevPositionSampler", renderData.gbuffer->getBuffer(GBufferTextures::POSITION, true));
				accumulator.descriptorSets[2]->setTexture("uPrevNormalSampler", renderData.gbuffer->getBuffer(GBufferTextures::NORMALS, true));
				accumulator.descriptorSets[2]->setTexture("uPrevVelocitySampler", renderData.gbuffer->getBuffer(GBufferTextures::VELOCITY, true));
				accumulator.descriptorSets[2]->setTexture("uPrevDepthSampler", renderData.gbuffer->getDepthBufferPong());

				for (auto set : accumulator.descriptorSets)
				{
					set->update(renderData.commandBuffer);
				}

				if (auto pushConsts = accumulator.reprojectionShader->getPushConstant(0))
				{
					pushConsts->setData(&accumulator.pushConsts);
				}

				accumulator.reprojectionPipeline->bind(renderData.commandBuffer);
				accumulator.reprojectionShader->bindPushConstants(renderData.commandBuffer, accumulator.reprojectionPipeline.get());
				Renderer::bindDescriptorSets(accumulator.reprojectionPipeline.get(), renderData.commandBuffer, 0, accumulator.descriptorSets);
				auto x = static_cast<uint32_t>(ceil(float(winSize.width) / float(TEMPORAL_ACCUMULATION_NUM_THREADS_X)));
				auto y = static_cast<uint32_t>(ceil(float(winSize.height) / float(TEMPORAL_ACCUMULATION_NUM_THREADS_Y)));
				Renderer::dispatch(renderData.commandBuffer, x, y, 1);

				accumulator.reprojectionPipeline->bufferBarrier(renderData.commandBuffer,
				                                                {accumulator.denoiseDispatchBuffer,
				                                                 accumulator.denoiseTileCoordsBuffer,
				                                                 accumulator.copyDispatchBuffer,
				                                                 accumulator.copyTileCoordsBuffer},
				                                                true);
			}

			inline auto atrousFilter(
			    component::AtrousFiler &        atrous,
			    component::ReflectionPipeline & pipeline,
			    maple::component::RendererData &renderData,
			    component::TemporalAccumulator &accumulator)
			{
				bool    pingPong = false;
				int32_t readIdx  = 0;
				int32_t writeIdx = 1;

				atrous.gBufferSet->setTexture("uNormalSampler", renderData.gbuffer->getBuffer(GBufferTextures::NORMALS));
				atrous.gBufferSet->setTexture("uLinearZSampler", renderData.gbuffer->getBuffer(GBufferTextures::LINEARZ));
				atrous.gBufferSet->update(renderData.commandBuffer);
				atrous.inputSet->update(renderData.commandBuffer);
				atrous.argsSet->update(renderData.commandBuffer);
				for (int32_t i = 0; i < atrous.iterations; i++)
				{
					readIdx  = (int32_t) pingPong;
					writeIdx = (int32_t) !pingPong;

					renderData.renderDevice->clearRenderTarget(atrous.atrousFilter[writeIdx], renderData.commandBuffer, {1, 1, 1, 1});

					atrous.copyTilesSet[writeIdx]->setTexture("outColor", atrous.atrousFilter[writeIdx]);
					atrous.copyTilesSet[writeIdx]->update(renderData.commandBuffer);

					//these coords should not denoise. so just set them as zero.
					{
						atrous.copyTilesPipeline->bind(renderData.commandBuffer);
						Renderer::bindDescriptorSets(atrous.copyTilesPipeline.get(), renderData.commandBuffer, 0, {atrous.copyTilesSet[writeIdx]});
						atrous.copyTilesPipeline->dispatchIndirect(renderData.commandBuffer, accumulator.copyDispatchBuffer.get());
						atrous.copyTilesPipeline->end(renderData.commandBuffer);
					}

					{
						atrous.copyWriteDescriptorSet[writeIdx]->setTexture("outColor", atrous.atrousFilter[writeIdx]);
						atrous.copyReadDescriptorSet[readIdx]->setTexture("uInput", atrous.atrousFilter[readIdx]);

						atrous.copyWriteDescriptorSet[writeIdx]->update(renderData.commandBuffer);
						atrous.copyReadDescriptorSet[readIdx]->update(renderData.commandBuffer);
					}

					{
						atrous.atrousFilerPipeline->bind(renderData.commandBuffer);
						auto pushConsts     = atrous.pushConsts;
						pushConsts.stepSize = 1 << i;
						pushConsts.power    = i == (atrous.iterations - 1) ? atrous.pushConsts.power : 0.0f;
						if (auto ptr = atrous.atrousFilerPipeline->getShader()->getPushConstant(0))
						{
							ptr->setData(&pushConsts);
							atrous.atrousFilerPipeline->getShader()->bindPushConstants(renderData.commandBuffer, atrous.atrousFilerPipeline.get());
						}

						//the first time(i is zero) set the accumulation's output as current input
						Renderer::bindDescriptorSets(
						    atrous.atrousFilerPipeline.get(),
						    renderData.commandBuffer, 0,
						    {atrous.copyWriteDescriptorSet[writeIdx],
						     atrous.gBufferSet,
						     i == 0 ? atrous.inputSet : atrous.copyReadDescriptorSet[readIdx],
						     atrous.argsSet});

						atrous.atrousFilerPipeline->dispatchIndirect(renderData.commandBuffer, accumulator.denoiseDispatchBuffer.get());
						atrous.atrousFilerPipeline->end(renderData.commandBuffer);
					}

					pingPong = !pingPong;
				}

				return atrous.atrousFilter[writeIdx];
			}

			inline auto system(Entity                                entity,
			                   const maple::component::WindowSize &  winSize,
			                   const maple::component::RendererData &rendererData,
			                   const maple::component::CameraView &  cameraView)
			{
				auto [acc] = entity;
				resetArgs(acc, rendererData);
			}
		}        // namespace denoise

		namespace frame_end
		{
			// clang-format off
			using Entity = ecs::Registry
				::Modify<raytraced_reflection::component::ReflectionPipeline>
				::To<ecs::Entity>;
			// clang-format on

			inline auto system(Entity entity)
			{
				auto [pipeline] = entity;
				pipeline.pushConsts.numFrames++;
			}
		}        // namespace frame_end

		namespace delegates
		{
			inline auto createAtrous(maple::Entity entity, ecs::World world,
			                         const component::ReflectionPipeline & pipeline,
			                         const component::TemporalAccumulator &accumulator)
			{
				auto &atrous  = entity.addComponent<component::AtrousFiler>();
				auto &winSize = world.getComponent<maple::component::WindowSize>();
				{
					atrous.atrousFilerShader = Shader::create("shaders/Reflection/DenoiseAtrous.shader");
					atrous.copyTilesShader   = Shader::create("shaders/Reflection/DenoiseCopyTiles.shader");

					PipelineInfo info1;
					info1.pipelineName         = "Atrous-Filer Reflection Pipeline";
					info1.shader               = atrous.atrousFilerShader;
					atrous.atrousFilerPipeline = Pipeline::get(info1);

					info1.pipelineName       = "Atrous-Filer Copy Tiles";
					info1.shader             = atrous.copyTilesShader;
					atrous.copyTilesPipeline = Pipeline::get(info1);

					atrous.gBufferSet = DescriptorSet::create({1, atrous.atrousFilerShader.get()});
					atrous.argsSet    = DescriptorSet::create({3, atrous.atrousFilerShader.get()});
					atrous.argsSet->setStorageBuffer("DenoiseTileData", accumulator.denoiseTileCoordsBuffer);
					atrous.argsSet->update(world.getComponent<maple::component::RendererData>().commandBuffer);

					for (uint32_t i = 0; i < 2; i++)
					{
						atrous.atrousFilter[i] = Texture2D::create();
						atrous.atrousFilter[i]->buildTexture(TextureFormat::RGBA16, winSize.width, winSize.height);
						atrous.atrousFilter[i]->setName("A-Trous Filter " + std::to_string(i));

						atrous.copyWriteDescriptorSet[i] = DescriptorSet::create({0, atrous.atrousFilerShader.get()});
						atrous.copyWriteDescriptorSet[i]->setTexture("outColor", atrous.atrousFilter[i]);
						atrous.copyWriteDescriptorSet[i]->setName("Atrous-Write-Descriptor-" + std::to_string(i));

						atrous.copyReadDescriptorSet[i] = DescriptorSet::create({2, atrous.atrousFilerShader.get()});
						atrous.copyReadDescriptorSet[i]->setName("Atrous-Read-Descriptor-" + std::to_string(i));

						atrous.copyTilesSet[i] = DescriptorSet::create({0, atrous.copyTilesShader.get()});
						atrous.copyTilesSet[i]->setStorageBuffer("ShadowTileData", accumulator.copyTileCoordsBuffer);
					}

					atrous.inputSet = DescriptorSet::create({2, atrous.atrousFilerShader.get()});
					atrous.inputSet->setTexture("uInput", pipeline.outColor);
				}
			}

			inline auto &createTemporalAccumulator(maple::Entity entity, ecs::World world)
			{
				auto &winSize = world.getComponent<maple::component::WindowSize>();

				auto &accumulator = entity.addComponent<component::TemporalAccumulator>();

				auto bufferSize =
				    sizeof(glm::ivec2) * static_cast<uint32_t>(ceil(float(winSize.width) / float(TEMPORAL_ACCUMULATION_NUM_THREADS_X))) *
				    static_cast<uint32_t>(ceil(float(winSize.height) / float(TEMPORAL_ACCUMULATION_NUM_THREADS_Y)));

				accumulator.resetArgsShader    = Shader::create("shaders/Reflection/DenoiseReset.shader");
				accumulator.reprojectionShader = Shader::create("shaders/Reflection/Reprojection.shader");

				PipelineInfo info;
				info.shader               = accumulator.resetArgsShader;
				info.pipelineName         = "Reflection-DenoiseReset";
				accumulator.resetPipeline = Pipeline::get(info);

				info.shader                      = accumulator.reprojectionShader;
				info.pipelineName                = "Reflection-Reprojection";
				accumulator.reprojectionPipeline = Pipeline::get(info);

				//###############

				PipelineInfo info2;
				info2.pipelineName        = "Reset-Args";
				info2.shader              = accumulator.resetArgsShader;
				accumulator.resetPipeline = Pipeline::get(info2);

				accumulator.denoiseTileCoordsBuffer = StorageBuffer::create(bufferSize, nullptr);
				accumulator.denoiseDispatchBuffer   = StorageBuffer::create(sizeof(int32_t) * 3, nullptr, BufferOptions{true, (int32_t) MemoryUsage::MEMORY_USAGE_GPU_ONLY});

				accumulator.copyTileCoordsBuffer = StorageBuffer::create(bufferSize, nullptr);
				accumulator.copyDispatchBuffer   = StorageBuffer::create(sizeof(int32_t) * 3, nullptr, BufferOptions{true, (int32_t) MemoryUsage::MEMORY_USAGE_GPU_ONLY});

				accumulator.indirectDescriptorSet = DescriptorSet::create({0, accumulator.resetArgsShader.get()});
				accumulator.indirectDescriptorSet->setStorageBuffer("DenoiseTileDispatchArgs", accumulator.denoiseDispatchBuffer);
				accumulator.indirectDescriptorSet->setStorageBuffer("CopyTileDispatchArgs", accumulator.copyDispatchBuffer);
				accumulator.indirectDescriptorSet->update(world.getComponent<maple::component::RendererData>().commandBuffer);

				accumulator.outputs[0] = Texture2D::create();
				accumulator.outputs[0]->buildTexture(TextureFormat::RGBA16, winSize.width, winSize.height);
				accumulator.outputs[0]->setName("Shadows Re-projection Output Ping");

				//RGB:Visibility - A:Variance
				accumulator.outputs[1] = Texture2D::create();
				accumulator.outputs[1]->buildTexture(TextureFormat::RGBA16, winSize.width, winSize.height);
				accumulator.outputs[1]->setName("Shadows Re-projection Output Pong");

				for (int32_t i = 0; i < 2; i++)
				{
					//Visibility - Visibility * Visibility HistoryLength.
					accumulator.currentMoments[i] = Texture2D::create();
					accumulator.currentMoments[i]->buildTexture(TextureFormat::RGBA16, winSize.width, winSize.height);
					accumulator.currentMoments[i]->setName("Shadows Re-projection Moments " + std::to_string(i));
				}
				return accumulator;
			}

			inline auto init(raytraced_reflection::component::RaytracedReflection &reflection, maple::Entity entity, ecs::World world)
			{
				auto &winSize     = world.getComponent<maple::component::WindowSize>();
				reflection.width  = winSize.width;
				reflection.height = winSize.height;

				auto &pipeline  = entity.addComponent<component::ReflectionPipeline>();
				auto &blueNoise = world.getComponent<blue_noise::global::component::BlueNoise>();

				pipeline.outColor = Texture2D::create();
				pipeline.outColor->buildTexture(TextureFormat::RGBA16, reflection.width, reflection.height);
				pipeline.outColor->setName("Reflection Output");

				pipeline.traceShader = Shader::create("shaders/Reflection/Reflection.shader", {{"VertexBuffer", MAX_SCENE_MESH_INSTANCE_COUNT},
				                                                                               {"IndexBuffer", MAX_SCENE_MESH_INSTANCE_COUNT},
				                                                                               {"SubmeshInfoBuffer", MAX_SCENE_MESH_INSTANCE_COUNT},
				                                                                               {"uSamplers", MAX_SCENE_MATERIAL_TEXTURE_COUNT}});

				pipeline.gbufferSet = DescriptorSet::create({6, pipeline.traceShader.get()});
				pipeline.noiseSet   = DescriptorSet::create({7, pipeline.traceShader.get()});
				pipeline.outSet     = DescriptorSet::create({8, pipeline.traceShader.get()});

				pipeline.outSet->setTexture("outColor", pipeline.outColor);
				pipeline.noiseSet->setTexture("uSobolSequence", blueNoise.sobolSequence);
				pipeline.noiseSet->setTexture("uScramblingRankingTile", blueNoise.scramblingRanking[blue_noise::Blue_Noise_1SPP]);

				PipelineInfo info;
				info.pipelineName         = "Reflection-Pipeline";
				info.shader               = pipeline.traceShader;
				info.maxRayRecursionDepth = 1;
				pipeline.tracePipeline    = Pipeline::get(info);

				/*
				*TODO denoise reflection... because GBuffer is not satisfied.
				*auto &acc = createTemporalAccumulator(entity, world);
				*createAtrous(entity, world, pipeline, acc);
				*/
			}
		}        // namespace delegates

		auto registerRaytracedReflection(ExecuteQueue &begin, ExecuteQueue &queue, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->onConstruct<raytraced_reflection::component::RaytracedReflection, delegates::init>();

			executePoint->registerWithinQueue<raytraced_reflection::update::system>(begin);
			executePoint->registerWithinQueue<raytraced_reflection::render::system>(queue);
			executePoint->registerWithinQueue<raytraced_reflection::denoise::system>(queue);

			executePoint->registerSystemInFrameEnd<raytraced_reflection::frame_end::system>();
		}
	};        // namespace raytraced_reflection
};            // namespace maple