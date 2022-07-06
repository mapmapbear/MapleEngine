//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "RaytracedShadow.h"
#include "RHI/DescriptorSet.h"
#include "RHI/RenderDevice.h"
#include "RHI/StorageBuffer.h"
#include "RHI/Texture.h"

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
			Texture2D::Ptr     prev;        //Shadows Previous Re-projection
			Texture2D::Ptr     output;
			Texture2D::Ptr     currentMoments[2];
			StorageBuffer::Ptr denoiseTileCoordsBuffer;
			StorageBuffer::Ptr denoiseDispatchBuffer;        // indirect?

			StorageBuffer::Ptr shadowTileCoordsBuffer;
			StorageBuffer::Ptr shadowDispatchBuffer;        // indirect?

			DescriptorSet::Ptr writeDescriptorSet;        //Shadows Ray Trace Write
			DescriptorSet::Ptr readDescriptorSet;         //Shadows Ray Trace Read
		};
	}        // namespace component

	namespace init
	{
		inline auto initRaytracedShadow(raytraced_shadow::component::RaytracedShadow &shadow, Entity entity, ecs::World world)
		{
			float scaleDivisor = powf(2.0f, float(shadow.scale));
			auto &winSize      = world.getComponent<component::WindowSize>();
			shadow.width       = winSize.width / scaleDivisor;
			shadow.height      = winSize.height / scaleDivisor;

			auto &pipeline = entity.addComponent<component::RaytraceShadowPipeline>();

			//////////////////////////////////////////////////////////
			pipeline.prev = Texture2D::create();
			pipeline.prev->setName("Shadows Previous Re-projection");
			pipeline.prev->buildTexture(TextureFormat::RG16F, shadow.width, shadow.height);

			pipeline.output = Texture2D::create();
			pipeline.output->setName("Shadows Re-projection Output");
			pipeline.output->buildTexture(TextureFormat::RG16F, shadow.width, shadow.height);

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

			pipeline.denoiseTileCoordsBuffer = StorageBuffer::create(bufferSize, nullptr, BufferOptions{false, true});
			pipeline.denoiseDispatchBuffer   = StorageBuffer::create(sizeof(int32_t) * 3, nullptr, BufferOptions{true, true});

			pipeline.shadowTileCoordsBuffer = StorageBuffer::create(bufferSize, nullptr, BufferOptions{false, true});
			pipeline.shadowDispatchBuffer   = StorageBuffer::create(sizeof(int32_t) * 3, nullptr, BufferOptions{true, true});
		}
	}        // namespace init

	namespace on_trace
	{
		using Entity = ecs::Registry ::Modify<raytraced_shadow::component::RaytracedShadow>::To<ecs::Entity>;

		inline auto system(Entity entity, component::RendererData &renderData)
		{
			//clear
			//trace...
		}
	}        // namespace on_trace

	namespace denoise
	{
		using Entity = ecs::Registry ::Modify<raytraced_shadow::component::RaytracedShadow>::To<ecs::Entity>;

		inline auto system(Entity entity, component::RendererData &renderData)
		{
			//reset 
			//temporal accumulation
			//a trous filter
			//upsample to fullscreen
		}
	}


	namespace raytraced_shadow
	{
		auto registerRaytracedShadow(ExecuteQueue &update, ExecuteQueue &queue, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->onConstruct<raytraced_shadow::component::RaytracedShadow, init::initRaytracedShadow>();

			executePoint->registerWithinQueue<on_trace::system>(queue);
		};
	}        // namespace raytraced_shadow
};           // namespace maple