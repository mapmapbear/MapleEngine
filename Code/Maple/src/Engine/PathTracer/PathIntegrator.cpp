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

			StorageBuffer::Ptr  lightBuffer;
			StorageBuffer::Ptr  materialBuffer;
			StorageBuffer::Ptr  transformBuffer;
			DescriptorPool::Ptr descriptorPool;

			DescriptorSet::Ptr sceneDescriptor;
			DescriptorSet::Ptr vboDescriptor;
			DescriptorSet::Ptr iboDescriptor;
			DescriptorSet::Ptr materialDescriptor;
			DescriptorSet::Ptr textureDescriptor;

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

			Material::Ptr defaultMaterial;

			std::vector<Texture::Ptr> shaderTextures;

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

			pipeline.pipeline        = Pipeline::get(info);
			pipeline.lightBuffer     = StorageBuffer::create(sizeof(component::LightData) * MAX_SCENE_LIGHT_COUNT, nullptr, BufferOptions{false, (int32_t) MemoryUsage::MEMORY_USAGE_CPU_TO_GPU, 0});
			pipeline.materialBuffer  = StorageBuffer::create(sizeof(raytracing::MaterialData) * MAX_SCENE_MATERIAL_COUNT, nullptr, BufferOptions{false, (int32_t) MemoryUsage::MEMORY_USAGE_CPU_TO_GPU, 0});
			pipeline.transformBuffer = StorageBuffer::create(sizeof(raytracing::TransformData) * MAX_SCENE_MESH_INSTANCE_COUNT, nullptr, BufferOptions{false, (int32_t) MemoryUsage::MEMORY_USAGE_CPU_TO_GPU, 0});

			pipeline.descriptorPool = DescriptorPool::create({25,
			                                                  {{DescriptorType::UniformBufferDynamic, 10},
			                                                   {DescriptorType::ImageSampler, MAX_SCENE_MATERIAL_TEXTURE_COUNT},
			                                                   {DescriptorType::Buffer, 5 * MAX_SCENE_MESH_INSTANCE_COUNT},
			                                                   {DescriptorType::AccelerationStructure, 10}}});

			pipeline.sceneDescriptor    = DescriptorSet::create({0, pipeline.shader.get(), 1, pipeline.descriptorPool.get()});
			pipeline.vboDescriptor      = DescriptorSet::create({1, pipeline.shader.get(), 1, pipeline.descriptorPool.get(), MAX_SCENE_MESH_INSTANCE_COUNT});
			pipeline.iboDescriptor      = DescriptorSet::create({2, pipeline.shader.get(), 1, pipeline.descriptorPool.get(), MAX_SCENE_MESH_INSTANCE_COUNT});
			pipeline.materialDescriptor = DescriptorSet::create({3, pipeline.shader.get(), 1, pipeline.descriptorPool.get(), MAX_SCENE_MESH_INSTANCE_COUNT});
			pipeline.textureDescriptor  = DescriptorSet::create({4, pipeline.shader.get(), 1, pipeline.descriptorPool.get(), MAX_SCENE_MATERIAL_TEXTURE_COUNT});
			pipeline.readDescriptor     = DescriptorSet::create({5, pipeline.shader.get()});
			pipeline.writeDescriptor    = DescriptorSet::create({6, pipeline.shader.get()});

			pipeline.defaultMaterial = std::make_shared<Material>();
			MaterialProperties properties;
			properties.albedoColor       = glm::vec4(1.f, 1.f, 1.f, 1.f);
			properties.roughnessColor    = glm::vec4(0);
			properties.metallicColor     = glm::vec4(0);
			properties.usingAlbedoMap    = 0.0f;
			properties.usingRoughnessMap = 0.0f;
			properties.usingNormalMap    = 0.0f;
			properties.usingMetallicMap  = 0.0f;
			pipeline.defaultMaterial->setMaterialProperites(properties);

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

		using LightDefine = ecs::Registry ::Fetch<component::Transform>::Modify<component::Light>;

		using LightEntity = LightDefine ::To<ecs::Entity>;

		using LightGroup = LightDefine::To<ecs::Group>;

		using MeshQuery = ecs::Registry ::Modify<component::MeshRenderer>::Modify<component::Transform>::To<ecs::Group>;

		using SkyboxGroup = ecs::Registry::Fetch<component::SkyboxData>::To<ecs::Group>;

		inline auto updateMaterial(raytracing::MaterialData &              materialData,
		                           Material *                              material,
		                           global::component::Bindless &           bindless,
		                           std::vector<Texture::Ptr> &             shaderTextures,
		                           std::unordered_map<uint32_t, uint32_t> &textureIndices)
		{
			materialData.textureIndices0 = glm::ivec4(-1);
			materialData.textureIndices1 = glm::ivec4(-1);

			materialData.albedo    = material->getProperties().albedoColor;
			materialData.roughness = material->getProperties().roughnessColor;
			materialData.metalic   = material->getProperties().metallicColor;
			materialData.emissive  = material->getProperties().emissiveColor;

			materialData.usingValue0 = {
			    material->getProperties().usingAlbedoMap,
			    material->getProperties().usingMetallicMap,
			    material->getProperties().usingRoughnessMap,
			    0};

			materialData.usingValue1 = {
			    material->getProperties().usingNormalMap,
			    material->getProperties().usingAOMap,
			    material->getProperties().usingEmissiveMap,
			    material->getProperties().workflow};

			auto textures = material->getMaterialTextures();

			if (textures.albedo)
			{
				if (textureIndices.count(textures.albedo->getId()) == 0)
				{
					materialData.textureIndices0.x = shaderTextures.size();
					shaderTextures.emplace_back(textures.albedo);
					textureIndices.emplace(textures.albedo->getId(), materialData.textureIndices0.x);
				}
				else
				{
					materialData.textureIndices0.x = textureIndices[textures.albedo->getId()];
				}
			}

			if (textures.normal)
			{
				if (textureIndices.count(textures.normal->getId()) == 0)
				{
					materialData.textureIndices0.y = shaderTextures.size();
					shaderTextures.emplace_back(textures.normal);
					textureIndices.emplace(textures.normal->getId(), materialData.textureIndices0.y);
				}
				else
				{
					materialData.textureIndices0.y = textureIndices[textures.normal->getId()];
				}
			}

			if (textures.roughness)
			{
				if (textureIndices.count(textures.roughness->getId()) == 0)
				{
					materialData.textureIndices0.z = shaderTextures.size();
					shaderTextures.emplace_back(textures.roughness);
					textureIndices.emplace(textures.roughness->getId(), materialData.textureIndices0.z);
				}
				else
				{
					materialData.textureIndices0.z = textureIndices[textures.roughness->getId()];
				}
			}

			if (textures.metallic)
			{
				if (textureIndices.count(textures.metallic->getId()) == 0)
				{
					materialData.textureIndices0.w = shaderTextures.size();
					shaderTextures.emplace_back(textures.metallic);
					textureIndices.emplace(textures.metallic->getId(), materialData.textureIndices0.w);
				}
				else
				{
					materialData.textureIndices0.w = textureIndices[textures.metallic->getId()];
				}
			}

			if (textures.emissive)
			{
				if (textureIndices.count(textures.emissive->getId()) == 0)
				{
					materialData.textureIndices1.x = shaderTextures.size();
					shaderTextures.emplace_back(textures.emissive);
					textureIndices.emplace(textures.emissive->getId(), materialData.textureIndices1.x);
				}
				else
				{
					materialData.textureIndices1.x = textureIndices[textures.emissive->getId()];
				}
			}

			if (textures.ao)
			{
				if (textureIndices.count(textures.ao->getId()) == 0)
				{
					materialData.textureIndices1.y = shaderTextures.size();
					shaderTextures.emplace_back(textures.ao);
					textureIndices.emplace(textures.ao->getId(), materialData.textureIndices1.y);
				}
				else
				{
					materialData.textureIndices1.y = textureIndices[textures.ao->getId()];
				}
			}
		}

		inline auto system(
		    Entity                                           entity,
		    LightGroup                                       lightGroup,
		    MeshQuery                                        meshGroup,
		    SkyboxGroup                                      skyboxGroup,
		    const maple::component::RendererData &           rendererData,
		    const raytracing::global::component::TopLevelAs &topLevels,
		    global::component::Bindless &                    bindless,
		    global::component::GraphicsContext &             context,
		    global::component::SceneTransformChanged *       sceneChanged,
		    global::component::MaterialChanged *             materialChanged)
		{
			auto [integrator, pipeline] = entity;
			if (sceneChanged && sceneChanged->dirty && integrator.enable && topLevels.topLevelAs)
			{
				std::unordered_set<uint32_t> processedMeshes;
				std::unordered_set<uint32_t> processedMaterials;
				//std::unordered_set<uint32_t>           processedTextures;
				std::unordered_map<uint32_t, uint32_t> globalMaterialIndices;
				uint32_t                               meshCounter        = 0;
				uint32_t                               gpuMaterialCounter = 0;

				std::vector<VertexBuffer::Ptr> vbos;
				std::vector<IndexBuffer::Ptr>  ibos;
				//std::vector<Texture::Ptr>       shaderTextures;
				std::vector<StorageBuffer::Ptr> materialIndices;

				context.context->waitIdle();

				auto meterialBuffer  = (raytracing::MaterialData *) pipeline.materialBuffer->map();
				auto transformBuffer = (raytracing::TransformData *) pipeline.transformBuffer->map();
				auto lightBuffer     = (component::LightData *) pipeline.lightBuffer->map();

				auto guard = sg::make_scope_guard([&]() {
					pipeline.materialBuffer->unmap();
					pipeline.lightBuffer->unmap();
					pipeline.transformBuffer->unmap();
				});

				auto tasks = BatchTask::create();

				for (auto meshEntity : meshGroup)
				{
					auto [mesh, transform] = meshGroup.convert(meshEntity);

					if (processedMeshes.count(mesh.mesh->getId()) == 0)
					{

						processedMeshes.emplace(mesh.mesh->getId());
						vbos.emplace_back(mesh.mesh->getVertexBuffer());
						ibos.emplace_back(mesh.mesh->getIndexBuffer());

						MAPLE_ASSERT(mesh.mesh->getSubMeshCount() != 0, "sub mesh should be one at least");

						for (auto i = 0; i < mesh.mesh->getSubMeshCount(); i++)
						{
							auto material = mesh.mesh->getMaterial(i);

							if (material == nullptr)
							{
								material = pipeline.defaultMaterial.get();
							}

							if (processedMaterials.count(material->getId()) == 0)
							{
								processedMaterials.emplace(material->getId());

								auto &materialData = meterialBuffer[gpuMaterialCounter++];

								updateMaterial(materialData, material, bindless, pipeline.shaderTextures, bindless.textureIndices);

								bindless.materialIndices[material->getId()] = gpuMaterialCounter - 1;
								//TODO if current is emissive, add an extra light here.
							}
						}
					}

					auto buffer = mesh.mesh->getSubMeshesBuffer();

					auto indices = (glm::uvec2 *) buffer->map();
					materialIndices.emplace_back(buffer);

					for (auto i = 0; i < mesh.mesh->getSubMeshCount(); i++)
					{
						auto material = mesh.mesh->getMaterial(i);
						if (material == nullptr)
						{
							material = pipeline.defaultMaterial.get();
						}
						const auto &subIndex = mesh.mesh->getSubMeshIndex();
						indices[i]           = glm::uvec2(i == 0 ? 0 : subIndex[i - 1] / 3, bindless.materialIndices[material->getId()]);
					}

					auto blas = mesh.mesh->getAccelerationStructure(tasks);

					auto meshIdx = bindless.meshIndices[mesh.mesh->getId()];

					transformBuffer[meshIdx].meshIndex    = meshIdx;
					transformBuffer[meshIdx].model        = transform.getWorldMatrix();
					transformBuffer[meshIdx].normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform.getWorldMatrix())));

					auto guard = sg::make_scope_guard([&]() {
						buffer->unmap();
					});
				}

				uint32_t lightIndicator = 0;

				for (auto lightEntity : lightGroup)
				{
					auto [transform, light]       = lightGroup.convert(lightEntity);
					light.lightData.position      = {transform.getWorldPosition(), 1.f};
					light.lightData.direction     = {glm::normalize(transform.getWorldOrientation() * maple::FORWARD), 1.f};
					lightBuffer[lightIndicator++] = light.lightData;
				}

				pipeline.sceneDescriptor->setTexture("uSkybox", rendererData.unitCube);
				if (!skyboxGroup.empty())
				{
					for (auto sky : skyboxGroup)
					{
						auto [skybox] = skyboxGroup.convert(sky);
						if (skybox.skybox != nullptr)
						{
							pipeline.sceneDescriptor->setTexture("uSkybox", skybox.skybox);
							lightBuffer[lightIndicator++].type = (float) component::LightType::EnvironmentLight;
						}
					}
				}

				pipeline.constants.numLights  = lightIndicator;
				pipeline.constants.maxBounces = integrator.maxBounces;

				tasks->execute();

				pipeline.sceneDescriptor->setStorageBuffer("MaterialBuffer", pipeline.materialBuffer);
				pipeline.sceneDescriptor->setStorageBuffer("TransformBuffer", pipeline.transformBuffer);
				pipeline.sceneDescriptor->setStorageBuffer("LightBuffer", pipeline.lightBuffer);
				pipeline.sceneDescriptor->setAccelerationStructure("uTopLevelAS", topLevels.topLevelAs);

				pipeline.vboDescriptor->setStorageBuffer("VertexBuffer", vbos);
				pipeline.iboDescriptor->setStorageBuffer("IndexBuffer", ibos);

				pipeline.materialDescriptor->setStorageBuffer("SubmeshInfoBuffer", materialIndices);
				pipeline.textureDescriptor->setTexture("uSamplers", pipeline.shaderTextures);
			}

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

		inline auto system(Entity                                entity,
		                   const maple::component::RendererData &rendererData,
		                   maple::component::CameraView &        cameraView,
		                   maple::component::WindowSize &        winSize,
		                   ecs::World                            world)
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

			pipeline.sceneDescriptor->update(rendererData.commandBuffer);
			pipeline.vboDescriptor->update(rendererData.commandBuffer);
			pipeline.iboDescriptor->update(rendererData.commandBuffer);
			pipeline.materialDescriptor->update(rendererData.commandBuffer);
			pipeline.textureDescriptor->update(rendererData.commandBuffer);
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
			                                 pipeline.sceneDescriptor,
			                                 pipeline.vboDescriptor,
			                                 pipeline.iboDescriptor,
			                                 pipeline.materialDescriptor,
			                                 pipeline.textureDescriptor,
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
