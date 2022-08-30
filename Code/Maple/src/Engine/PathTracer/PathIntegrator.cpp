//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "PathIntegrator.h"
#include "Engine/CaptureGraph.h"
#include "Engine/Mesh.h"
#include "Engine/Raytrace/AccelerationStructure.h"
#include "Engine/Renderer/RendererData.h"

#include "RHI/AccelerationStructure.h"
#include "RHI/DescriptorPool.h"
#include "RHI/GraphicsContext.h"
#include "RHI/Pipeline.h"
#include "RHI/RenderDevice.h"
#include "RHI/StorageBuffer.h"

#include "Scene/Component/Bindless.h"
#include "Scene/Component/Light.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Component/Transform.h"
#include "Scene/System/BindlessModule.h"

#include "Engine/Renderer/SkyboxRenderer.h"

#include "TracedData.h"

#include <scope_guard.hpp>

#include "Application.h"

#ifdef MAPLE_VULKAN
#	include "RHI/Vulkan/Vk.h"
#endif        // MAPLE_VULKAN

namespace maple
{
	namespace
	{
		constexpr uint32_t MAX_SCENE_MESH_INSTANCE_COUNT    = 1024;
		constexpr uint32_t MAX_SCENE_LIGHT_COUNT            = 300;
		constexpr uint32_t MAX_SCENE_MATERIAL_COUNT         = 4096;
		constexpr uint32_t MAX_SCENE_MATERIAL_TEXTURE_COUNT = MAX_SCENE_MATERIAL_COUNT * 4;
	}        // namespace

	namespace component
	{
		struct PathTracePipeline
		{
			Pipeline::Ptr pipeline;
			Shader::Ptr   shader;

			DescriptorSet::Ptr readDescriptor;
			DescriptorSet::Ptr writeDescriptor;

			struct PushConstants
			{
				glm::mat4 invViewProj;
				glm::vec4 cameraPos;
				glm::vec4 upDirection;
				glm::vec4 rightDirection;
				uint32_t  numFrames;
				uint32_t  maxBounces;
				uint32_t  numLights;
				float     accumulation;
				float     shadowRayBias;
				float     padding0;
				float     padding1;
				float     padding2;
			} constants;

			glm::mat4 projView;
		};
	}        // namespace component

	namespace init
	{
		inline auto initPathIntegrator(component::PathIntegrator &path, Entity entity, ecs::World world)
		{
			auto &pipeline  = entity.addComponent<component::PathTracePipeline>();
			pipeline.shader = Shader::create("shaders/PathTrace/PathTrace.shader", {{"VertexBuffer", MAX_SCENE_MESH_INSTANCE_COUNT},
			                                                                        {"IndexBuffer", MAX_SCENE_MESH_INSTANCE_COUNT},
			                                                                        {"SubmeshInfoBuffer", MAX_SCENE_MESH_INSTANCE_COUNT},
			                                                                        {"uSamplers", MAX_SCENE_MATERIAL_TEXTURE_COUNT}});
			PipelineInfo info;
			info.shader               = pipeline.shader;
			info.maxRayRecursionDepth = 8;
			info.pipelineName         = "PathTrace";

			pipeline.pipeline = Pipeline::get(info);

			pipeline.readDescriptor  = DescriptorSet::create({5, pipeline.shader.get()});
			pipeline.writeDescriptor = DescriptorSet::create({6, pipeline.shader.get()});

			auto &winSize = world.getComponent<component::WindowSize>();

			for (auto i = 0; i < 2; i++)
			{
				path.images[i] = Texture2D::create();
				path.images[i]->buildTexture(TextureFormat::RGBA32, winSize.width, winSize.height);
			}
		}
	}        // namespace init

	namespace gather_scene
	{
		using Entity = ecs::Registry::Fetch<component::PathIntegrator>::Modify<component::PathTracePipeline>::To<ecs::Entity>;

		inline auto system(Entity entity, const global::component::RaytracingDescriptor &descriptor)
		{
			auto [integrator, pipeline] = entity;

			pipeline.constants.numLights  = descriptor.numLights;
			pipeline.constants.maxBounces = integrator.maxBounces;

			/*if (materialChanged && pipeline.tlas->isBuilt() && materialChanged->updateQueue.size() > 0)
			{
				auto meterialBuffer = (raytracing::MaterialData *) pipeline.materialBuffer->map();

				for (auto material : materialChanged->updateQueue)
				{
					if (auto iter = bindless.materialIndices.find(material->getId()); iter == bindless.materialIndices.end())
					{
						bindless.materialIndices[material->getId()] = bindless.materialIndices.size();
					}

					auto  index = bindless.materialIndices[material->getId()];
					auto &data  = meterialBuffer[index];
					updateMaterial(data, material, bindless, pipeline.shaderTextures, bindless.textureIndices);
				}

				pipeline.textureDescriptor->setTexture("uSamplers", pipeline.shaderTextures);
				pipeline.sceneDescriptor->setStorageBuffer("MaterialBuffer", pipeline.materialBuffer);

				pipeline.materialBuffer->unmap();
			}*/
		}
	}        // namespace gather_scene

	namespace tracing
	{
		using Entity = ecs::Registry::Modify<component::PathIntegrator>::Modify<component::PathTracePipeline>::To<ecs::Entity>;

		inline auto system(Entity                                   entity,
		                   const maple::component::RendererData &   rendererData,
		                   maple::component::CameraView &           cameraView,
		                   maple::component::WindowSize &           winSize,
		                   global::component::RaytracingDescriptor &descriptor,
		                   ecs::World                               world)
		{
			auto [integrator, pipeline] = entity;

			if (!integrator.enable)
				return;

			if (cameraView.projView != pipeline.projView)
			{
				pipeline.projView             = cameraView.projView;
				integrator.accumulatedSamples = 0;
			}

			pipeline.readDescriptor->setTexture("uPreviousColor", integrator.images[integrator.readIndex]);
			pipeline.writeDescriptor->setTexture("uCurrentColor", integrator.images[1 - integrator.readIndex]);

			integrator.readIndex = 1 - integrator.readIndex;
		
			pipeline.readDescriptor->update(rendererData.commandBuffer);
			pipeline.writeDescriptor->update(rendererData.commandBuffer);

			pipeline.pipeline->bind(rendererData.commandBuffer);

			glm::vec3 right   = cameraView.cameraTransform->getRightDirection();
			glm::vec3 up      = cameraView.cameraTransform->getUpDirection();
			glm::vec3 forward = cameraView.cameraTransform->getForwardDirection();

			pipeline.constants.cameraPos      = glm::vec4{cameraView.cameraTransform->getWorldPosition(), 0};
			pipeline.constants.upDirection    = glm::vec4(up, 0.0f);
			pipeline.constants.rightDirection = glm::vec4(right, 0.0f);
			pipeline.constants.invViewProj    = glm::inverse(cameraView.projView);
			pipeline.constants.numFrames      = integrator.accumulatedSamples++;
			pipeline.constants.accumulation   = float(pipeline.constants.numFrames) / float(pipeline.constants.numFrames + 1);
			pipeline.constants.shadowRayBias  = integrator.shadowRayBias;
			
			for (auto &pushConsts : pipeline.pipeline->getShader()->getPushConstants())
			{
				pushConsts.setData(&pipeline.constants);
			}
			pipeline.pipeline->getShader()->bindPushConstants(rendererData.commandBuffer, pipeline.pipeline.get());

			Renderer::bindDescriptorSets(pipeline.pipeline.get(), rendererData.commandBuffer, 0,
			                             {
			                                 descriptor.sceneDescriptor,
			                                 descriptor.vboDescriptor,
			                                 descriptor.iboDescriptor,
			                                 descriptor.materialDescriptor,
			                                 descriptor.textureDescriptor,
			                                 pipeline.readDescriptor,
			                                 pipeline.writeDescriptor,
			                             });

			pipeline.pipeline->traceRays(rendererData.commandBuffer, winSize.width, winSize.height, 1);
		}
	}        // namespace tracing

	namespace path_integrator
	{
		auto registerPathIntegrator(ExecuteQueue &begin, ExecuteQueue &renderer, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->onConstruct<component::PathIntegrator, init::initPathIntegrator>();
			executePoint->registerWithinQueue<gather_scene::system>(begin);
			executePoint->registerWithinQueue<tracing::system>(renderer);
		}
	}        // namespace path_integrator
}        // namespace maple
