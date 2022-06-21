//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "PathIntegrator.h"
#include "RHI/GraphicsContext.h"
#include "RHI/Pipeline.h"
#include "RHI/StorageBuffer.h"

#include "Engine/Mesh.h"
#include "Scene/Component/IndirectDraw.h"
#include "Scene/Component/Light.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Component/Transform.h"

#include "TracedData.h"

#include <scope_guard.hpp>

#define MAX_SCENE_MESH_INSTANCE_COUNT 1024
#define MAX_SCENE_LIGHT_COUNT 300
#define MAX_SCENE_MATERIAL_COUNT 4096
#define MAX_SCENE_MATERIAL_TEXTURE_COUNT (MAX_SCENE_MATERIAL_COUNT * 4)

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
		}
	}        // namespace init

	namespace gather_scene
	{
		// clang-format off
		using Entity = ecs::Registry 
			::Fetch<component::PathIntegrator>
			::Fetch<component::PathTracePipeline>
			::To<ecs::Entity>;

		using LightDefine = ecs::Registry 
			::Fetch<component::Transform>
			::Modify<component::Light>;

		using LightEntity = LightDefine 
			::To<ecs::Entity>;

		using LightGroup = LightDefine 
			::To<ecs::Group>;

		using MeshQuery = ecs::Registry 
			::Modify<component::MeshRenderer>
			::Modify<component::Transform>
			::To<ecs::Group>;
		// clang-format on

		inline auto system(
		    Entity                                    entity,
		    LightGroup                                lightGroup,
		    MeshQuery                                 meshGroup,
		    global::component::IndirectDraw &         indirectDraw,
		    global::component::GraphicsContext &      context,
		    global::component::SceneTransformChanged *sceneChanged)
		{
			auto [integrator, pipeline] = entity;
			if (sceneChanged->dirty)
			{
				std::unordered_set<uint32_t>           processedMeshes;
				std::unordered_set<uint32_t>           processedMaterials;
				std::unordered_set<uint32_t>           processedTextures;
				std::unordered_map<uint32_t, uint32_t> globalMaterialIndices;
				uint32_t                               meshCounter        = 0;
				uint32_t                               gpuMaterialCounter = 0;

				std::vector<VertexBuffer::Ptr> vbos;
				std::vector<IndexBuffer::Ptr>  ibos;
				std::vector<Texture::Ptr>      shaderTextures;

				context.context->waitIdle();

				auto meterialBuffer = (raytracing::MaterialData *) pipeline.materialBuffer->map();
				auto guard          = sg::make_scope_guard([&]() {
                    pipeline.materialBuffer->unmap();
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
