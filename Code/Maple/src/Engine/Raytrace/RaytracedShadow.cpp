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
			Texture2D::Ptr prev;        //Shadows Previous Re-projection
			Texture2D::Ptr output;
			Texture2D::Ptr currentMoments[2];

			DescriptorSet::Ptr writeDescriptorSet;        //Shadows Ray Trace Write
			DescriptorSet::Ptr readDescriptorSet;         //Shadows Ray Trace Read

			bool firstFrame = false;

			Pipeline::Ptr pipeline;

			Shader::Ptr shadowRaytraceShader;

			float showBias = 0.5f;
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
			pipeline.prev = Texture2D::create();
			pipeline.prev->buildTexture(TextureFormat::RG16F, shadow.width, shadow.height);
			pipeline.prev->setName("Shadows Previous Re-projection");

			pipeline.output = Texture2D::create();
			pipeline.output->buildTexture(TextureFormat::RG16F, shadow.width, shadow.height);
			pipeline.output->setName("Shadows Re-projection Output");

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
			

			PipelineInfo info;
			info.shader               = accumulator.resetArgsShader;
			accumulator.resetPipeline = Pipeline::get(info);

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
		using Entity = ecs::Registry 
			::Modify<raytraced_shadow::component::RaytracedShadow>::Modify<component::TemporalAccumulator>::To<ecs::Entity>;

		inline auto reserArgs(component::TemporalAccumulator &acc, const component::RendererData &renderData)
		{
			acc.indirectDescriptorSet->update(renderData.commandBuffer);
			acc.resetPipeline->bind(renderData.commandBuffer);
			Renderer::bindDescriptorSets(acc.resetPipeline.get(), renderData.commandBuffer, 0, {acc.indirectDescriptorSet});
			Renderer::dispatch(renderData.commandBuffer, 1, 1, 1);
		}

		inline auto accumulation(component::TemporalAccumulator &acc, const component::RendererData &renderData)
		{
			
		}

		inline auto system(Entity entity, component::RendererData &renderData)
		{
			auto [shadow, acc] = entity;
			//reset
			//temporal accumulation
			//a trous filter
			//upsample to fullscreen

			reserArgs(acc, renderData);
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