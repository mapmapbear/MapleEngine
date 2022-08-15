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
		constexpr uint32_t RAY_TRACE_NUM_THREADS_X             = 8;
		constexpr uint32_t RAY_TRACE_NUM_THREADS_Y             = 4;
		constexpr uint32_t TEMPORAL_ACCUMULATION_NUM_THREADS_X = 8;
		constexpr uint32_t TEMPORAL_ACCUMULATION_NUM_THREADS_Y = 8;

	}        // namespace

	namespace component
	{
		struct RaytraceShadowPipeline
		{
			Texture2D::Ptr raytraceImage;
			Texture2D::Ptr upsample;
			Texture2D::Ptr atrousFilter[2];        //A-Trous Filter

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

			DescriptorSet::Ptr indirectDescriptorSet;
			Shader::Ptr        resetArgsShader;
			Pipeline::Ptr      resetPipeline;

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
			pipeline.outputs[0]->buildTexture(TextureFormat::R32F, shadow.width, shadow.height);
			pipeline.outputs[0]->setName("Shadows Re-projection Output Ping");

			pipeline.outputs[1] = Texture2D::create();
			pipeline.outputs[1]->buildTexture(TextureFormat::R32F, shadow.width, shadow.height);
			pipeline.outputs[1]->setName("Shadows Re-projection Output Pong");

			for (int32_t i = 0; i < 2; i++)
			{
				pipeline.currentMoments[i] = Texture2D::create();
				pipeline.currentMoments[i]->buildTexture(TextureFormat::RGBA16, shadow.width, shadow.height);
				pipeline.currentMoments[i]->setName("Shadows Re-projection Moments " + std::to_string(i));
			}

			//////////////////////////////////////////////////////////
			pipeline.raytraceImage = Texture2D::create();
			pipeline.raytraceImage->setName("Shadows Ray Trace");
			pipeline.raytraceImage->buildTexture(TextureFormat::R32UI,
			                                     static_cast<uint32_t>(ceil(float(shadow.width) / float(RAY_TRACE_NUM_THREADS_X))),
			                                     static_cast<uint32_t>(ceil(float(shadow.height) / float(RAY_TRACE_NUM_THREADS_Y))));
			for (int32_t i = 0; i < 2; i++)
			{
				pipeline.atrousFilter[i] = Texture2D::create();
				pipeline.atrousFilter[i]->buildTexture(TextureFormat::RG16F, shadow.width, shadow.height);
				pipeline.atrousFilter[i]->setName("A-Trous Filter " + std::to_string(i));
			}

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
			PipelineInfo info;
			info.shader                      = accumulator.reprojectionShader;
			accumulator.reprojectionPipeline = Pipeline::get(info);
			//###############

			PipelineInfo info2;
			info2.shader              = accumulator.resetArgsShader;
			accumulator.resetPipeline = Pipeline::get(info2);

			////////////////////////////////////////////////////////////////////////////////////////

			pipeline.firstFrame           = true;
			pipeline.shadowRaytraceShader = Shader::create("shaders/Shadow/ShadowRaytrace.shader");
			pipeline.writeDescriptorSet   = DescriptorSet::create({0, pipeline.shadowRaytraceShader.get()});
			pipeline.readDescriptorSet    = DescriptorSet::create({0, pipeline.shadowRaytraceShader.get()});
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
			::To<ecs::Entity>;
		// clang-format on

		inline auto reserArgs(component::TemporalAccumulator &acc, const component::RendererData &renderData)
		{
			acc.indirectDescriptorSet->update(renderData.commandBuffer);
			acc.resetPipeline->bind(renderData.commandBuffer);
			Renderer::bindDescriptorSets(acc.resetPipeline.get(), renderData.commandBuffer, 0, {acc.indirectDescriptorSet});
			Renderer::dispatch(renderData.commandBuffer, 1, 1, 1);
		}

		inline auto accumulation(component::TemporalAccumulator &   accumulator,
		                         component::RaytraceShadowPipeline &pipeline,
		                         const component::RendererData &    renderData,
		                         const component::WindowSize &      winSize,
		                         const component::CameraView &      cameraView)
		{
			accumulator.descriptorSets[0]->setTexture("outColor", pipeline.outputs[pipeline.pingPong]);
			accumulator.descriptorSets[0]->setTexture("moment", pipeline.currentMoments[pipeline.pingPong]);
			accumulator.descriptorSets[0]->setTexture("uHistoryOutput", pipeline.outputs[1 - pipeline.pingPong]);        //prev
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

		inline auto system(Entity entity, component::RendererData &renderData, const component::WindowSize &winSize, const component::CameraView & cameraView)
		{
			auto [shadow, acc, pipeline] = entity;
			//reset
			//temporal accumulation
			//a trous filter
			//upsample to fullscreen
			reserArgs(acc, renderData);
			accumulation(acc, pipeline, renderData, winSize, cameraView);

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