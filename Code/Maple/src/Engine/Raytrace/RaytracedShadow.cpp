//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "RaytracedShadow.h"
#include "RHI/DescriptorSet.h"
#include "RHI/Pipeline.h"
#include "RHI/RenderDevice.h"
#include "RHI/StorageBuffer.h"
#include "RHI/Texture.h"

#include "Scene/Component/Light.h"
#include "Scene/Component/Transform.h"

#include "Engine/CaptureGraph.h"
#include "Engine/Noise/BlueNoise.h"
#include "Engine/Raytrace/AccelerationStructure.h"

#include "Engine/GBuffer.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/RendererData.h"

#include <glm/gtc/type_ptr.hpp>

namespace maple
{
	namespace
	{
		//using int32_t to pack visibility
		constexpr uint32_t RAY_TRACE_NUM_THREADS_X = 8;
		constexpr uint32_t RAY_TRACE_NUM_THREADS_Y = 4;

		constexpr uint32_t TEMPORAL_ACCUMULATION_NUM_THREADS_X = 8;
		constexpr uint32_t TEMPORAL_ACCUMULATION_NUM_THREADS_Y = 8;

	}        // namespace

	namespace component
	{
		struct RaytraceShadowPipeline
		{
			Texture2D::Ptr raytraceImage;
			Texture2D::Ptr upsample;

			//Temporal - Accumulation
			Texture2D::Ptr outputs[2];        //Shadows Previous Re-projection
			Texture2D::Ptr currentMoments[2];

			DescriptorSet::Ptr writeDescriptorSet;        //Shadows Ray Trace Write
			DescriptorSet::Ptr readDescriptorSet;         //Shadows Ray Trace Read

			bool firstFrame = false;

			Pipeline::Ptr pipeline;

			Shader::Ptr shadowRaytraceShader;

			float showBias = 0.5f;

			int32_t pingPong = 0;
		};

		struct TemporalAccumulator
		{
			StorageBuffer::Ptr denoiseTileCoordsBuffer;
			StorageBuffer::Ptr denoiseDispatchBuffer;        // indirect?

			StorageBuffer::Ptr shadowTileCoordsBuffer;
			StorageBuffer::Ptr shadowDispatchBuffer;        // indirect?

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
			} pushConsts;

			int32_t iterations = 4;

			Shader::Ptr   atrousFilerShader;
			Pipeline::Ptr atrousFilerPipeline;

			Shader::Ptr   copyTilesShader;
			Pipeline::Ptr copyTilesPipeline;

			DescriptorSet::Ptr copyWriteDescriptorSet[2];
			DescriptorSet::Ptr copyReadDescriptorSet[2];

			DescriptorSet::Ptr copyTilesSet[2];

			DescriptorSet::Ptr inputSet;
			DescriptorSet::Ptr gBufferSet;
			DescriptorSet::Ptr argsSet;

			Texture2D::Ptr atrousFilter[2];        //A-Trous Filter
		};

	}        // namespace component

	namespace init
	{
		inline auto initRaytracedShadow(raytraced_shadow::component::RaytracedShadow &shadow, Entity entity, ecs::World world)
		{
			float scaleDivisor = std::powf(2.0f, float(shadow.scale));
			auto &winSize      = world.getComponent<component::WindowSize>();
			shadow.width       = winSize.width / scaleDivisor;
			shadow.height      = winSize.height / scaleDivisor;

			auto &pipeline = entity.addComponent<component::RaytraceShadowPipeline>();

			//////////////////////////////////////////////////////////

			pipeline.outputs[0] = Texture2D::create();
			pipeline.outputs[0]->buildTexture(TextureFormat::RG16F, shadow.width, shadow.height);
			pipeline.outputs[0]->setName("Shadows Re-projection Output Ping");
			//R:Visibility - G:Variance
			pipeline.outputs[1] = Texture2D::create();
			pipeline.outputs[1]->buildTexture(TextureFormat::RG16F, shadow.width, shadow.height);
			pipeline.outputs[1]->setName("Shadows Re-projection Output Pong");

			for (int32_t i = 0; i < 2; i++)
			{
				//Visibility - Visibility * Visibility HistoryLength.
				pipeline.currentMoments[i] = Texture2D::create();
				pipeline.currentMoments[i]->buildTexture(TextureFormat::RGBA16, shadow.width, shadow.height);
				pipeline.currentMoments[i]->setName("Shadows Re-projection Moments " + std::to_string(i));
			}

			//////////////////////////////////////////////////////////
			auto w = static_cast<uint32_t>(ceil(float(shadow.width) / float(RAY_TRACE_NUM_THREADS_X)));
			auto h = static_cast<uint32_t>(ceil(float(shadow.height) / float(RAY_TRACE_NUM_THREADS_Y)));

			pipeline.raytraceImage = Texture2D::create(w, h, nullptr, {TextureFormat::R32UI, TextureFilter::Nearest});
			pipeline.raytraceImage->setName("Shadows Ray Trace");

			pipeline.upsample = Texture2D::create();
			pipeline.upsample->buildTexture(TextureFormat::R16, winSize.width, winSize.height);
			pipeline.upsample->setName("upsample");
			////////////////////////Create Buffers///////////////////////

			auto bufferSize = sizeof(glm::ivec2) * static_cast<uint32_t>(ceil(float(shadow.width) / float(TEMPORAL_ACCUMULATION_NUM_THREADS_X))) * static_cast<uint32_t>(ceil(float(shadow.height) / float(TEMPORAL_ACCUMULATION_NUM_THREADS_Y)));

			auto &accumulator = entity.addComponent<component::TemporalAccumulator>();

			accumulator.resetArgsShader = Shader::create("shaders/Shadow/DenoiseReset.shader");

			accumulator.denoiseTileCoordsBuffer = StorageBuffer::create(bufferSize, nullptr);
			accumulator.denoiseDispatchBuffer   = StorageBuffer::create(sizeof(int32_t) * 3, nullptr, BufferOptions{true});

			accumulator.shadowTileCoordsBuffer = StorageBuffer::create(bufferSize, nullptr);
			accumulator.shadowDispatchBuffer   = StorageBuffer::create(sizeof(int32_t) * 3, nullptr, BufferOptions{true});

			accumulator.indirectDescriptorSet = DescriptorSet::create({0, accumulator.resetArgsShader.get()});
			accumulator.indirectDescriptorSet->setStorageBuffer("DenoiseTileDispatchArgs", accumulator.denoiseTileCoordsBuffer);
			accumulator.indirectDescriptorSet->setStorageBuffer("ShadowTileDispatchArgs", accumulator.shadowDispatchBuffer);
			accumulator.indirectDescriptorSet->update(world.getComponent<component::RendererData>().commandBuffer);

			//###############
			accumulator.reprojectionShader = Shader::create("shaders/Shadow/DenoiseReprojection.shader");
			constexpr char *str[4]         = {"Accumulation", "GBuffer", "PrevGBuffer", "Args"};
			accumulator.descriptorSets.resize(4);
			for (uint32_t i = 0; i < 4; i++)
			{
				accumulator.descriptorSets[i] = DescriptorSet::create({i, accumulator.reprojectionShader.get()});
				accumulator.descriptorSets[i]->setName(str[i]);
			}

			accumulator.descriptorSets[3]->setStorageBuffer("DenoiseTileData", accumulator.denoiseDispatchBuffer);
			accumulator.descriptorSets[3]->setStorageBuffer("DenoiseTileDispatchArgs", accumulator.denoiseTileCoordsBuffer);
			accumulator.descriptorSets[3]->setStorageBuffer("ShadowTileData", accumulator.shadowTileCoordsBuffer);
			accumulator.descriptorSets[3]->setStorageBuffer("ShadowTileDispatchArgs", accumulator.shadowDispatchBuffer);
			accumulator.descriptorSets[3]->update(world.getComponent<component::RendererData>().commandBuffer);

			PipelineInfo info;
			info.shader                      = accumulator.reprojectionShader;
			info.pipelineName                = "Reprojection";
			accumulator.reprojectionPipeline = Pipeline::get(info);
			
			//###############

			PipelineInfo info2;
			info2.pipelineName        = "Reset-Args";
			info2.shader              = accumulator.resetArgsShader;
			accumulator.resetPipeline = Pipeline::get(info2);

			////////////////////////////////////////////////////////////////////////////////////////

			pipeline.firstFrame           = true;
			pipeline.shadowRaytraceShader = Shader::create("shaders/Shadow/ShadowRaytrace.shader");
			pipeline.writeDescriptorSet   = DescriptorSet::create({0, pipeline.shadowRaytraceShader.get()});
			pipeline.readDescriptorSet    = DescriptorSet::create({0, pipeline.shadowRaytraceShader.get()});

			auto &atrous = entity.addComponent<component::AtrousFiler>();
			{
				atrous.atrousFilerShader = Shader::create("shaders/Shadow/DenoiseAtrous.shader");
				atrous.copyTilesShader   = Shader::create("shaders/Shadow/DenoiseCopyTiles.shader");

				PipelineInfo info1;
				info2.pipelineName = "Atrous-Filer Pipeline";
				info1.shader               = atrous.atrousFilerShader;
				atrous.atrousFilerPipeline = Pipeline::get(info1);

				info1.shader             = atrous.copyTilesShader;
				atrous.copyTilesPipeline = Pipeline::get(info1);

				atrous.gBufferSet = DescriptorSet::create({1, atrous.atrousFilerShader.get()});
				atrous.argsSet    = DescriptorSet::create({3, atrous.atrousFilerShader.get()});
				atrous.argsSet->setStorageBuffer("DenoiseTileData", accumulator.denoiseDispatchBuffer);
				atrous.argsSet->update(world.getComponent<component::RendererData>().commandBuffer);

				for (uint32_t i = 0; i < 2; i++)
				{
					atrous.atrousFilter[i] = Texture2D::create();
					atrous.atrousFilter[i]->buildTexture(TextureFormat::RG16F, shadow.width, shadow.height);
					atrous.atrousFilter[i]->setName("A-Trous Filter " + std::to_string(i));

					atrous.copyWriteDescriptorSet[i] = DescriptorSet::create({0, atrous.atrousFilerShader.get()});
					atrous.copyWriteDescriptorSet[i]->setTexture("outColor", atrous.atrousFilter[i]);

					atrous.copyReadDescriptorSet[i] = DescriptorSet::create({2, atrous.atrousFilerShader.get()});
					atrous.copyReadDescriptorSet[i]->setTexture("uInput", atrous.atrousFilter[i]);

					atrous.copyTilesSet[i] = DescriptorSet::create({0, atrous.copyTilesShader.get()});
					atrous.copyTilesSet[i]->setTexture("outColor", atrous.atrousFilter[i]);
					atrous.copyTilesSet[i]->setStorageBuffer("ShadowTileData", accumulator.shadowTileCoordsBuffer);
				}

				atrous.inputSet = DescriptorSet::create({2, atrous.atrousFilerShader.get()});
				atrous.inputSet->setTexture("uInput", pipeline.outputs[0]);
			}
		}
	}        // namespace init

	namespace on_trace
	{
		using Entity = ecs::Registry ::Modify<raytraced_shadow::component::RaytracedShadow>::Modify<component::RaytraceShadowPipeline>::To<ecs::Entity>;

		using LightDefine = ecs::Registry ::Modify<component::Light>::Fetch<component::Transform>;

		using LightEntity = LightDefine ::To<ecs::Entity>;

		using LightGroup = LightDefine ::To<ecs::Group>;

		inline auto begin(Entity entity, LightGroup group)
		{
			auto [shadow, pipeline] = entity;

			component::LightData lights[32] = {};
			uint32_t             numLights  = 0;
			{
				PROFILE_SCOPE("Get Light");
				group.forEach([&](LightEntity entity) {
					auto [light, transform] = entity;

					light.lightData.position  = {transform.getWorldPosition(), 1.f};
					light.lightData.direction = {glm::normalize(transform.getWorldOrientation() * maple::FORWARD), 1.f};

					lights[numLights] = light.lightData;
					numLights++;
				});
			}
			pipeline.writeDescriptorSet->setUniform("UniformBufferObject", "light", lights, sizeof(component::LightData), false);
		}

		inline auto render(Entity                                           entity,
		                   component::RendererData &                        renderData,
		                   maple::capture_graph::component::RenderGraph &   graph,
		                   const raytracing::global::component::TopLevelAs &topLevel,
		                   const blue_noise::global::component::BlueNoise & blueNoise)
		{
			if (topLevel.topLevelAs == nullptr)
			{
				return;
			}

			auto [shadow, pipeline] = entity;

			pipeline.writeDescriptorSet->setTexture("uPositionSampler", renderData.gbuffer->getBuffer(GBufferTextures::POSITION));
			pipeline.writeDescriptorSet->setTexture("uNormalSampler", renderData.gbuffer->getBuffer(GBufferTextures::NORMALS));
			pipeline.writeDescriptorSet->setTexture("uDepthSampler", renderData.gbuffer->getDepthBuffer());
			pipeline.writeDescriptorSet->setTexture("uSobolSequence", blueNoise.sobolSequence);
			pipeline.writeDescriptorSet->setTexture("uScramblingRankingTile", blueNoise.scramblingRanking[blue_noise::Blue_Noise_1SPP]);
			pipeline.writeDescriptorSet->setTexture("outColor", pipeline.raytraceImage);
			pipeline.writeDescriptorSet->setAccelerationStructure("uTopLevelAS", topLevel.topLevelAs);

			pipeline.writeDescriptorSet->update(renderData.commandBuffer);

			auto &pushConsts = pipeline.shadowRaytraceShader->getPushConstants();
			if (!pushConsts.empty())
			{
				struct
				{
					float    bias;
					uint32_t numFrames;
				} pushConstsStruct;
				pushConstsStruct.bias      = pipeline.showBias;
				pushConstsStruct.numFrames = renderData.numFrames;
				pushConsts[0].setData(&pushConstsStruct);
			}

			PipelineInfo info{};
			info.shader       = pipeline.shadowRaytraceShader;
			pipeline.pipeline = Pipeline::get(info, {pipeline.writeDescriptorSet}, graph);

			pipeline.pipeline->bind(renderData.commandBuffer);
			pipeline.shadowRaytraceShader->bindPushConstants(renderData.commandBuffer, pipeline.pipeline.get());
			Renderer::bindDescriptorSets(pipeline.pipeline.get(), renderData.commandBuffer, 0, {pipeline.writeDescriptorSet});
			Renderer::dispatch(renderData.commandBuffer,
			                   static_cast<uint32_t>(ceil(float(shadow.width) / float(RAY_TRACE_NUM_THREADS_X))),
			                   static_cast<uint32_t>(ceil(float(shadow.height) / float(RAY_TRACE_NUM_THREADS_Y))), 1);
			pipeline.pipeline->end(renderData.commandBuffer);
		}
	}        // namespace on_trace

	namespace denoise
	{
		// clang-format off
		using Entity = ecs::Registry 
			::Modify<raytraced_shadow::component::RaytracedShadow>
			::Modify<component::TemporalAccumulator>
			::Modify<component::RaytraceShadowPipeline>
			::Modify<component::AtrousFiler>
			::To<ecs::Entity>;
		// clang-format on

		inline auto reserArgs(component::TemporalAccumulator &acc, const component::RendererData &renderData)
		{
			acc.resetPipeline->bind(renderData.commandBuffer);
			Renderer::bindDescriptorSets(acc.resetPipeline.get(), renderData.commandBuffer, 0, {acc.indirectDescriptorSet});
			Renderer::dispatch(renderData.commandBuffer, 1, 1, 1);
			acc.resetPipeline->end(renderData.commandBuffer);
		}

		inline auto accumulation(component::TemporalAccumulator &   accumulator,
		                         component::RaytraceShadowPipeline &pipeline,
		                         const component::RendererData &    renderData,
		                         const component::WindowSize &      winSize,
		                         const component::CameraView &      cameraView)
		{
			accumulator.descriptorSets[0]->setTexture("outColor", pipeline.outputs[0]);
			accumulator.descriptorSets[0]->setTexture("moment", pipeline.currentMoments[pipeline.pingPong]);
			accumulator.descriptorSets[0]->setTexture("uHistoryOutput", pipeline.outputs[1]);        //prev
			accumulator.descriptorSets[0]->setTexture("uHistoryMoments", pipeline.currentMoments[1 - pipeline.pingPong]);
			accumulator.descriptorSets[0]->setTexture("uInput", pipeline.raytraceImage);        //noise shadow
			accumulator.descriptorSets[0]->setUniform("UniformBufferObject", "viewProjInv", glm::value_ptr(glm::inverse(cameraView.projView)));

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
		}

		inline auto atrousFilter(
		    component::AtrousFiler &           atrous,
		    component::RaytraceShadowPipeline &pipeline,
		    component::RendererData &          renderData,
		    component::TemporalAccumulator &   accumulator)
		{
			bool    pingPong = false;
			int32_t readIdx  = 0;
			int32_t writeIdx = 1;

			atrous.gBufferSet->setTexture("uPositionSampler", renderData.gbuffer->getBuffer(GBufferTextures::POSITION));
			atrous.gBufferSet->setTexture("uNormalSampler", renderData.gbuffer->getBuffer(GBufferTextures::NORMALS));
			atrous.gBufferSet->setTexture("uVelocitySampler", renderData.gbuffer->getBuffer(GBufferTextures::VELOCITY));
			atrous.gBufferSet->setTexture("uDepthSampler", renderData.gbuffer->getDepthBuffer());
			atrous.gBufferSet->update(renderData.commandBuffer);
			atrous.inputSet->update(renderData.commandBuffer);

			for (int32_t i = 0; i < atrous.iterations; i++)
			{
				atrous.copyTilesSet[writeIdx]->setTexture("outColor", atrous.atrousFilter[writeIdx]);
				atrous.copyWriteDescriptorSet[writeIdx]->setTexture("outColor", atrous.atrousFilter[writeIdx]);
				atrous.copyReadDescriptorSet[readIdx]->setTexture("uInput", atrous.atrousFilter[readIdx]);

				atrous.copyTilesSet[writeIdx]->update(renderData.commandBuffer);
				atrous.copyWriteDescriptorSet[writeIdx]->update(renderData.commandBuffer);
				atrous.copyReadDescriptorSet[readIdx]->update(renderData.commandBuffer);

				readIdx  = (int32_t) pingPong;
				writeIdx = (int32_t) !pingPong;
				renderData.renderDevice->clearRenderTarget(atrous.atrousFilter[writeIdx], renderData.commandBuffer, {1, 1, 1, 1});
				//Copy....
				atrous.copyTilesPipeline->bind(renderData.commandBuffer);
				Renderer::bindDescriptorSets(atrous.copyTilesPipeline.get(), renderData.commandBuffer, 0, {atrous.copyTilesSet[writeIdx]});
				atrous.copyTilesPipeline->dispatchIndirect(renderData.commandBuffer, accumulator.denoiseDispatchBuffer.get());
				atrous.copyTilesPipeline->end(renderData.commandBuffer);

				atrous.atrousFilerPipeline->bind(renderData.commandBuffer);
				auto pushConsts     = atrous.pushConsts;
				pushConsts.stepSize = 1 << i;
				pushConsts.power    = i == (atrous.iterations - 1) ? atrous.pushConsts.power : 0.0f;
				if (auto ptr = atrous.atrousFilerPipeline->getShader()->getPushConstant(0))
				{
					ptr->setData(&pushConsts);
					atrous.atrousFilerPipeline->getShader()->bindPushConstants(renderData.commandBuffer, atrous.atrousFilerPipeline.get());
				}

				Renderer::bindDescriptorSets(
				    atrous.atrousFilerPipeline.get(),
				    renderData.commandBuffer, 0,
				    {atrous.copyWriteDescriptorSet[writeIdx],
				     atrous.gBufferSet,
				     i == 0 ? atrous.inputSet : atrous.copyReadDescriptorSet[readIdx],
				     atrous.argsSet});

				atrous.atrousFilerPipeline->dispatchIndirect(renderData.commandBuffer, accumulator.denoiseDispatchBuffer.get());
				atrous.atrousFilerPipeline->end(renderData.commandBuffer);

				pingPong = !pingPong;

				if (i == 1)
				{
					Texture2D::copy(atrous.atrousFilter[writeIdx], pipeline.outputs[1], renderData.commandBuffer);
				}
			}

			return atrous.atrousFilter[writeIdx];
		}

		inline auto system(Entity entity, component::RendererData &renderData, const component::WindowSize &winSize, const component::CameraView &cameraView)
		{
			auto [shadow, acc, pipeline, atrous] = entity;
			//reset
			//temporal accumulation
			//a trous filter
			//upsample to fullscreen
			reserArgs(acc, renderData);
			accumulation(acc, pipeline, renderData, winSize, cameraView);
			shadow.output = pipeline.outputs[0];

				/*	shadow.output = atrousFilter(atrous, pipeline, renderData, acc);*/
			
			pipeline.pingPong = 1 - pipeline.pingPong;
		}
	}        // namespace denoise

	namespace raytraced_shadow
	{
		auto registerRaytracedShadow(ExecuteQueue &update, ExecuteQueue &queue, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->onConstruct<raytraced_shadow::component::RaytracedShadow, init::initRaytracedShadow>();

			executePoint->registerGlobalComponent<blue_noise::global::component::BlueNoise>([](auto &noise) {
				noise.sobolSequence = Texture2D::create(blue_noise::SOBOL_TEXTURE, blue_noise::SOBOL_TEXTURE);
				for (int32_t i = 0; i < blue_noise ::BlueNoiseSpp::Length; i++)
				{
					noise.scramblingRanking[i] = Texture2D::create(blue_noise::SCRAMBLING_RANKING_TEXTURES[i], blue_noise::SCRAMBLING_RANKING_TEXTURES[i]);
				}
			});

			executePoint->registerWithinQueue<on_trace::begin>(update);

			executePoint->registerWithinQueue<on_trace::render>(queue);
			executePoint->registerWithinQueue<denoise::system>(queue);
		};
	}        // namespace raytraced_shadow
};           // namespace maple