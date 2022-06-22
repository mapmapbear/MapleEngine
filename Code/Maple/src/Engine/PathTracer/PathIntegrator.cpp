//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "PathIntegrator.h"
#include "RHI/AccelerationStructure.h"
#include "RHI/GraphicsContext.h"
#include "RHI/Pipeline.h"
#include "RHI/StorageBuffer.h"

#include "Engine/Mesh.h"
#include "Scene/Component/IndirectDraw.h"
#include "Scene/Component/Light.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Component/Transform.h"

#include "Engine/Renderer/SkyboxRenderer.h"

#include "TracedData.h"

#include <scope_guard.hpp>

#define MAX_SCENE_MESH_INSTANCE_COUNT 1024
#define MAX_SCENE_LIGHT_COUNT 300
#define MAX_SCENE_MATERIAL_COUNT 4096
#define MAX_SCENE_MATERIAL_TEXTURE_COUNT (MAX_SCENE_MATERIAL_COUNT * 4)

#ifdef MAPLE_VULKAN
#	include "RHI/Vulkan/Vk.h"
#endif        // MAPLE_VULKAN

namespace maple
{
	namespace component
	{
		struct PathTracePipeline
		{
			Pipeline::Ptr pipeline;
			Shader::Ptr   shader;

			StorageBuffer::Ptr lightBuffer;
			StorageBuffer::Ptr materialBuffer;
			StorageBuffer::Ptr transformBuffer;
		};

		struct AccelerationStructureData        // topLevel -> whole scene.
		{
			AccelerationStructure::Ptr tlas;
			StorageBuffer::Ptr         instanceBufferHost;
			StorageBuffer::Ptr         instanceBufferDevice;
			StorageBuffer::Ptr         scratchBuffer;
			bool                       built = false;
		};

	}        // namespace component

	namespace init
	{
		inline auto initPathIntegrator(component::PathIntegrator &path, Entity entity, ecs::World world)
		{
			auto &pipeline  = entity.addComponent<component::PathTracePipeline>();
			pipeline.shader = Shader::create("shaders/PathTrace/PathTrace.shader");
			PipelineInfo info;
			info.shader = pipeline.shader;

			pipeline.pipeline        = Pipeline::get(info);
			pipeline.lightBuffer     = StorageBuffer::create();
			pipeline.materialBuffer  = StorageBuffer::create();
			pipeline.transformBuffer = StorageBuffer::create();

			pipeline.lightBuffer->setData(sizeof(component::LightData) * MAX_SCENE_LIGHT_COUNT, nullptr);
			pipeline.materialBuffer->setData(sizeof(raytracing::MaterialData) * MAX_SCENE_MATERIAL_COUNT, nullptr);
			pipeline.materialBuffer->setData(sizeof(raytracing::TransformData) * MAX_SCENE_MESH_INSTANCE_COUNT, nullptr);

			auto &tlas = entity.addComponent<component::AccelerationStructureData>();

#ifdef MAPLE_VULKAN
			tlas.instanceBufferDevice = StorageBuffer::create(
			    sizeof(VkAccelerationStructureInstanceKHR) * MAX_SCENE_MESH_INSTANCE_COUNT,
			    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
			        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
			        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			    {false, VMA_MEMORY_USAGE_GPU_ONLY, 0});

			tlas.instanceBufferHost = StorageBuffer::create(
			    sizeof(VkAccelerationStructureInstanceKHR) * MAX_SCENE_MESH_INSTANCE_COUNT,
			    VK_BUFFER_USAGE_TRANSFER_SRC_BIT, {false,VMA_MEMORY_USAGE_CPU_ONLY, VMA_ALLOCATION_CREATE_MAPPED_BIT});

			tlas.tlas = AccelerationStructure::createTopLevel(tlas.instanceBufferDevice->getDeviceAddress(), MAX_SCENE_MESH_INSTANCE_COUNT);

			tlas.scratchBuffer = StorageBuffer::create(tlas.tlas->getBuildScratchSize(),
			                                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			                                           {false, VMA_MEMORY_USAGE_GPU_ONLY, 0});
#endif
		}
	}        // namespace init

	namespace gather_scene
	{
		using Entity = ecs::Registry ::Fetch<component::PathIntegrator>::Fetch<component::PathTracePipeline>::To<ecs::Entity>;

		using LightDefine = ecs::Registry ::Fetch<component::Transform>::Modify<component::Light>;

		using LightEntity = LightDefine ::To<ecs::Entity>;

		using LightGroup = LightDefine::To<ecs::Group>;

		using MeshQuery = ecs::Registry ::Modify<component::MeshRenderer>::Modify<component::Transform>::To<ecs::Group>;

		using SkyboxGroup = ecs::Registry::Fetch<component::SkyboxData>::To<ecs::Group>;

		inline auto system(
		    Entity                                    entity,
		    LightGroup                                lightGroup,
		    MeshQuery                                 meshGroup,
		    global::component::IndirectDraw &         indirectDraw,
		    global::component::GraphicsContext &      context,
		    global::component::SceneTransformChanged *sceneChanged)
		{
			auto [integrator, pipeline] = entity;
			if (sceneChanged && sceneChanged->dirty)
			{
				std::unordered_set<uint32_t>           processedMeshes;
				std::unordered_set<uint32_t>           processedMaterials;
				std::unordered_set<uint32_t>           processedTextures;
				std::unordered_map<uint32_t, uint32_t> globalMaterialIndices;
				uint32_t                               meshCounter        = 0;
				uint32_t                               gpuMaterialCounter = 0;

				std::vector<VertexBuffer::Ptr>  vbos;
				std::vector<IndexBuffer::Ptr>   ibos;
				std::vector<Texture::Ptr>       shaderTextures;
				std::vector<StorageBuffer::Ptr> materialIndices;

				context.context->waitIdle();

				auto meterialBuffer  = (raytracing::MaterialData *) pipeline.materialBuffer->map();
				auto transformBuffer = (raytracing::MaterialData *) pipeline.transformBuffer->map();
				auto lightBuffer     = (raytracing::MaterialData *) pipeline.lightBuffer->map();

				auto guard = sg::make_scope_guard([&]() {
					pipeline.materialBuffer->unmap();
					pipeline.lightBuffer->unmap();
					pipeline.transformBuffer->unmap();
				});

				for (auto meshEntity : meshGroup)
				{
					auto [mesh, transform] = meshGroup.convert(meshEntity);

					if (processedMeshes.count(mesh.mesh->getId()) == 0)
					{
						processedMeshes.emplace(mesh.mesh->getId());
						indirectDraw.meshIndices[mesh.mesh->getId()] = meshCounter++;
						vbos.emplace_back(mesh.mesh->getVertexBuffer());
						ibos.emplace_back(mesh.mesh->getIndexBuffer());

						for (auto i = 0; i < mesh.mesh->getSubMeshCount(); i++)
						{
							auto material = mesh.mesh->getMaterial(i);

							if (processedMaterials.count(material->getId()) == 0)
							{
								processedMaterials.emplace(material->getId());
								auto &materialData           = meterialBuffer[gpuMaterialCounter++];
								materialData.textureIndices0 = glm::ivec4(-1);
								materialData.textureIndices1 = glm::ivec4(-1);

								materialData.albedo     = material->getProperties().albedoColor;
								materialData.roughness  = material->getProperties().roughnessColor;
								materialData.metalic    = material->getProperties().metallicColor;
								materialData.emissive   = material->getProperties().emissiveColor;
								materialData.emissive.a = material->getProperties().workflow;

								auto textures = material->getMaterialTextures();

								if (textures.albedo)
								{
									if (processedTextures.count(textures.albedo->getId()) == 0)
									{
										materialData.textureIndices0.x = shaderTextures.size();
										shaderTextures.emplace_back(textures.albedo);
									}
								}

								if (textures.normal)
								{
									if (processedTextures.count(textures.normal->getId()) == 0)
									{
										materialData.textureIndices0.y = shaderTextures.size();
										shaderTextures.emplace_back(textures.normal);
									}
								}

								if (textures.roughness)
								{
									if (processedTextures.count(textures.roughness->getId()) == 0)
									{
										materialData.textureIndices0.z = shaderTextures.size();
										shaderTextures.emplace_back(textures.roughness);
									}
								}

								if (textures.metallic)
								{
									if (processedTextures.count(textures.metallic->getId()) == 0)
									{
										materialData.textureIndices0.w = shaderTextures.size();
										shaderTextures.emplace_back(textures.metallic);
									}
								}

								if (textures.emissive)
								{
									if (processedTextures.count(textures.emissive->getId()) == 0)
									{
										materialData.textureIndices1.x = shaderTextures.size();
										shaderTextures.emplace_back(textures.emissive);
									}
								}

								if (textures.ao)
								{
									if (processedTextures.count(textures.ao->getId()) == 0)
									{
										materialData.textureIndices1.y = shaderTextures.size();
										shaderTextures.emplace_back(textures.ao);
									}
								}

								indirectDraw.materialIndices[material->getId()] = gpuMaterialCounter - 1;
								//TODO if current is emissive, add an extra light here.
							}
						}
					}

					auto buffer  = mesh.mesh->getSubMeshesBuffer();
					auto indices = (glm::uvec2 *) buffer->map();
					materialIndices.emplace_back(buffer);

					for (auto i = 0; i < mesh.mesh->getSubMeshCount(); i++)
					{
						auto        material = mesh.mesh->getMaterial(i);
						const auto &subIndex = mesh.mesh->getSubMeshIndex();
						indices[i]           = glm::uvec2(subIndex[i] / 3, indirectDraw.materialIndices[material->getId()]);
					}

					auto guard = sg::make_scope_guard([&]() {
						buffer->unmap();
					});
				}
			}
		}
	}        // namespace gather_scene

	namespace path_integrator
	{
		auto registerPathIntegrator(ExecuteQueue &begin, ExecuteQueue &renderer, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->onConstruct<component::PathIntegrator, init::initPathIntegrator>();
			executePoint->registerWithinQueue<gather_scene::system>(begin);
		}
	}        // namespace path_integrator
}        // namespace maple
