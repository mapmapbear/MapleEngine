//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "BindlessModule.h"

#include "Engine/Mesh.h"
#include "Engine/Raytrace/AccelerationStructure.h"
#include "Engine/Renderer/RendererData.h"
#include "Engine/Renderer/SkyboxRenderer.h"

#include "Scene/Component/Bindless.h"
#include "Scene/Component/Light.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Component/Transform.h"
#include "Scene/System/ExecutePoint.h"

#include "Engine/PathTracer/TracedData.h"

#include "RHI/AccelerationStructure.h"
#include "RHI/GraphicsContext.h"
#include "RHI/Shader.h"
#include "RHI/Texture.h"

#include <scope_guard.hpp>

namespace maple
{
	namespace
	{
		constexpr uint32_t MAX_SCENE_MESH_INSTANCE_COUNT    = 1024;
		constexpr uint32_t MAX_SCENE_LIGHT_COUNT            = 300;
		constexpr uint32_t MAX_SCENE_MATERIAL_COUNT         = 4096;
		constexpr uint32_t MAX_SCENE_MATERIAL_TEXTURE_COUNT = MAX_SCENE_MATERIAL_COUNT * 4;
	}        // namespace

	namespace bindless
	{
		namespace clear_queue
		{
			using Entity = ecs::Registry::Modify<global::component::MaterialChanged>::To<ecs::Entity>;
			inline auto system(Entity entity)
			{
				auto [changed] = entity;
				if (!changed.updateQueue.empty())
				{
					changed.updateQueue.clear();
				}
			}
		}        // namespace clear_queue

		namespace gather_scene
		{
			// clang-format off
		using Entity = ecs::Registry
			::Modify<global::component::RaytracingDescriptor>
			::To<ecs::Entity>;

		using LightDefine = ecs::Registry 
			::Fetch<component::Transform>
			::Modify<component::Light>;

		using LightEntity = LightDefine::To<ecs::Entity>;

		using LightGroup = LightDefine::To<ecs::Group>;

		using MeshQuery = ecs::Registry 
			::Modify<component::MeshRenderer>
			::Modify<component::Transform>
			::To<ecs::Group>;

		using SkyboxGroup = ecs::Registry
			::Fetch<component::SkyboxData>
			::To<ecs::Group>;
			// clang-format on

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

			inline auto system(Entity                                                  entity,
			                   LightGroup                                              lightGroup,
			                   MeshQuery                                               meshGroup,
			                   SkyboxGroup                                             skyboxGroup,
			                   const maple::component::RendererData &                  rendererData,
			                   const maple::raytracing::global::component::TopLevelAs &topLevels,
			                   global::component::Bindless &                           bindless,
			                   global::component::GraphicsContext &                    context,
			                   global::component::SceneTransformChanged *              sceneChanged,
			                   global::component::MaterialChanged *                    materialChanged)
			{
				auto [descriptor] = entity;
				if (sceneChanged && sceneChanged->dirty && topLevels.topLevelAs)
				{
					descriptor.updated = true;
					std::unordered_set<uint32_t>           processedMeshes;
					std::unordered_set<uint32_t>           processedMaterials;
					std::unordered_map<uint32_t, uint32_t> globalMaterialIndices;
					uint32_t                               meshCounter        = 0;
					uint32_t                               gpuMaterialCounter = 0;

					std::vector<VertexBuffer::Ptr> vbos;
					std::vector<IndexBuffer::Ptr>  ibos;
					//std::vector<Texture::Ptr>       shaderTextures;
					std::vector<StorageBuffer::Ptr> materialIndices;

					context.context->waitIdle();

					auto meterialBuffer  = (raytracing::MaterialData *) descriptor.materialBuffer->map();
					auto transformBuffer = (raytracing::TransformData *) descriptor.transformBuffer->map();
					auto lightBuffer     = (component::LightData *) descriptor.lightBuffer->map();

					auto guard = sg::make_scope_guard([&]() {
						descriptor.materialBuffer->unmap();
						descriptor.lightBuffer->unmap();
						descriptor.transformBuffer->unmap();
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
									material = descriptor.defaultMaterial.get();
								}

								if (processedMaterials.count(material->getId()) == 0)
								{
									processedMaterials.emplace(material->getId());

									auto &materialData = meterialBuffer[gpuMaterialCounter++];

									updateMaterial(materialData, material, bindless, descriptor.shaderTextures, bindless.textureIndices);

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
								material = descriptor.defaultMaterial.get();
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

					descriptor.sceneDescriptor->setTexture("uSkybox", rendererData.unitCube);
					if (!skyboxGroup.empty())
					{
						for (auto sky : skyboxGroup)
						{
							auto [skybox] = skyboxGroup.convert(sky);
							if (skybox.skybox != nullptr)
							{
								descriptor.sceneDescriptor->setTexture("uSkybox", skybox.skybox);
								lightBuffer[lightIndicator++].type = (float) component::LightType::EnvironmentLight;
							}
						}
					}
					descriptor.numLights = lightIndicator;

					tasks->execute();

					descriptor.sceneDescriptor->setStorageBuffer("MaterialBuffer", descriptor.materialBuffer);
					descriptor.sceneDescriptor->setStorageBuffer("TransformBuffer", descriptor.transformBuffer);
					descriptor.sceneDescriptor->setStorageBuffer("LightBuffer", descriptor.lightBuffer);
					descriptor.sceneDescriptor->setAccelerationStructure("uTopLevelAS", topLevels.topLevelAs);

					descriptor.vboDescriptor->setStorageBuffer("VertexBuffer", vbos);
					descriptor.iboDescriptor->setStorageBuffer("IndexBuffer", ibos);

					descriptor.materialDescriptor->setStorageBuffer("SubmeshInfoBuffer", materialIndices);
					descriptor.textureDescriptor->setTexture("uSamplers", descriptor.shaderTextures);
				}
			}
		}        // namespace gather_scene

		namespace update_command
		{
			// clang-format off
		using Entity = ecs::Registry
			::Modify<global::component::RaytracingDescriptor>
			::To<ecs::Entity>;
			// clang-format on

			inline auto system(Entity entity, const maple::component::RendererData &rendererData)
			{
				auto [descriptor] = entity;
				if (descriptor.updated)
				{
					descriptor.sceneDescriptor->update(rendererData.commandBuffer);
					descriptor.vboDescriptor->update(rendererData.commandBuffer);
					descriptor.iboDescriptor->update(rendererData.commandBuffer);
					descriptor.materialDescriptor->update(rendererData.commandBuffer);
					descriptor.textureDescriptor->update(rendererData.commandBuffer);
				}
			}
		}        // namespace update_command

		auto registerBindlessModule(ExecuteQueue &begin, ExecuteQueue &renderer, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerWithinQueue<gather_scene::system>(begin);
			executePoint->registerWithinQueue<update_command::system>(renderer);

			executePoint->registerGlobalComponent<global::component::RaytracingDescriptor>([](auto &descriptor) {
				auto shader = Shader::create("shaders/PathTrace/PathTrace.shader", {{"VertexBuffer", MAX_SCENE_MESH_INSTANCE_COUNT},
				                                                                    {"IndexBuffer", MAX_SCENE_MESH_INSTANCE_COUNT},
				                                                                    {"SubmeshInfoBuffer", MAX_SCENE_MESH_INSTANCE_COUNT},
				                                                                    {"uSamplers", MAX_SCENE_MATERIAL_TEXTURE_COUNT}});

				descriptor.lightBuffer     = StorageBuffer::create(sizeof(component::LightData) * MAX_SCENE_LIGHT_COUNT, nullptr, BufferOptions{false, (int32_t) MemoryUsage::MEMORY_USAGE_CPU_TO_GPU, 0});
				descriptor.materialBuffer  = StorageBuffer::create(sizeof(raytracing::MaterialData) * MAX_SCENE_MATERIAL_COUNT, nullptr, BufferOptions{false, (int32_t) MemoryUsage::MEMORY_USAGE_CPU_TO_GPU, 0});
				descriptor.transformBuffer = StorageBuffer::create(sizeof(raytracing::TransformData) * MAX_SCENE_MESH_INSTANCE_COUNT, nullptr, BufferOptions{false, (int32_t) MemoryUsage::MEMORY_USAGE_CPU_TO_GPU, 0});

				descriptor.descriptorPool = DescriptorPool::create({25,
				                                                    {{DescriptorType::UniformBufferDynamic, 10},
				                                                     {DescriptorType::ImageSampler, MAX_SCENE_MATERIAL_TEXTURE_COUNT},
				                                                     {DescriptorType::Buffer, 5 * MAX_SCENE_MESH_INSTANCE_COUNT},
				                                                     {DescriptorType::AccelerationStructure, 10}}});

				descriptor.sceneDescriptor    = DescriptorSet::create({0, shader.get(), 1, descriptor.descriptorPool.get()});
				descriptor.vboDescriptor      = DescriptorSet::create({1, shader.get(), 1, descriptor.descriptorPool.get(), MAX_SCENE_MESH_INSTANCE_COUNT});
				descriptor.iboDescriptor      = DescriptorSet::create({2, shader.get(), 1, descriptor.descriptorPool.get(), MAX_SCENE_MESH_INSTANCE_COUNT});
				descriptor.materialDescriptor = DescriptorSet::create({3, shader.get(), 1, descriptor.descriptorPool.get(), MAX_SCENE_MESH_INSTANCE_COUNT});
				descriptor.textureDescriptor  = DescriptorSet::create({4, shader.get(), 1, descriptor.descriptorPool.get(), MAX_SCENE_MATERIAL_TEXTURE_COUNT});

				descriptor.defaultMaterial = std::make_shared<Material>();
				MaterialProperties properties;
				properties.albedoColor       = glm::vec4(1.f, 1.f, 1.f, 1.f);
				properties.roughnessColor    = glm::vec4(0);
				properties.metallicColor     = glm::vec4(0);
				properties.usingAlbedoMap    = 0.0f;
				properties.usingRoughnessMap = 0.0f;
				properties.usingNormalMap    = 0.0f;
				properties.usingMetallicMap  = 0.0f;
				descriptor.defaultMaterial->setMaterialProperites(properties);
			});
		}

		auto registerBindless(std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerSystemInFrameEnd<clear_queue::system>();
		}
	}        // namespace bindless
}        // namespace maple