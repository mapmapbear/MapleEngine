//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "DDGIRenderer.h"
#include "Engine/CaptureGraph.h"
#include "Engine/Material.h"
#include "Engine/Mesh.h"
#include "Engine/PathTracer/TracedData.h"
#include "Engine/Raytrace/AccelerationStructure.h"
#include "Engine/Raytrace/RaytraceScale.h"
#include "Engine/Renderer/RendererData.h"
#include "Engine/Renderer/SkyboxRenderer.h"

#include "RHI/BatchTask.h"
#include "RHI/DescriptorPool.h"
#include "RHI/DescriptorSet.h"
#include "RHI/GraphicsContext.h"
#include "RHI/IndexBuffer.h"
#include "RHI/Pipeline.h"
#include "RHI/RenderDevice.h"
#include "RHI/StorageBuffer.h"
#include "RHI/Texture.h"
#include "RHI/VertexBuffer.h"

#include "Math/MathUtils.h"
#include "Others/Randomizer.h"

#include "Scene/Component/Bindless.h"
#include "Scene/Component/BoundingBox.h"
#include "Scene/Component/Light.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Component/Transform.h"

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

	namespace ddgi
	{
		namespace component
		{
			struct Running
			{
				int8_t dummy;
			};

			struct DDGIPipelineInternal
			{
				BoundingBox box = {
				    {-1.f, -1.f, -1.f},
				    {1.f, 1.f, 1.f}};

				//raytrace pass.
				Texture2D::Ptr radiance;
				Texture2D::Ptr directionDepth;

				Texture2D::Ptr irradiance[2];
				Texture2D::Ptr depth[2];

				//result
				Texture2D::Ptr sampleProbeGrid;
				int32_t        frames   = 0;
				int32_t        pingPong = 0;
				Randomizer     rand;
			};

			struct RaytracePass
			{
				Shader::Ptr   shader;
				Pipeline::Ptr pipeline;

				StorageBuffer::Ptr  lightBuffer;
				StorageBuffer::Ptr  materialBuffer;
				StorageBuffer::Ptr  transformBuffer;
				DescriptorPool::Ptr descriptorPool;

				DescriptorSet::Ptr sceneDescriptor;
				DescriptorSet::Ptr vboDescriptor;
				DescriptorSet::Ptr iboDescriptor;
				DescriptorSet::Ptr materialDescriptor;
				DescriptorSet::Ptr textureDescriptor;
				DescriptorSet::Ptr samplerDescriptor;
				DescriptorSet::Ptr outpuDescriptor;

				Material::Ptr             defaultMaterial;
				std::vector<Texture::Ptr> shaderTextures;

				struct PushConstants
				{
					glm::mat4 randomOrientation;
					uint32_t  numFrames;
					uint32_t  infiniteBounces = 1;
					int32_t   numLights;
					float     intensity;
				} pushConsts;
				bool firstFrame = true;
			};
		}        // namespace component

		namespace init
		{
			inline auto initializeProbeGrid(
			    ddgi::component::DDGIPipeline &        pipeline,
			    ddgi::component::DDGIPipelineInternal &internal,
			    component::DDGIUniform &               uniform,
			    ecs::World                             world)
			{
				uint32_t totalProbes = uniform.probeCounts.x * uniform.probeCounts.y * uniform.probeCounts.z;

				{
					internal.radiance = Texture2D::create();
					internal.radiance->setName("DDGI Raytrace Radiance");
					internal.radiance->buildTexture(TextureFormat::RGBA16, pipeline.raysPerProbe, totalProbes);

					internal.directionDepth = Texture2D::create();
					internal.directionDepth->setName("DDGI Raytrace Direction Depth");
					internal.directionDepth->buildTexture(TextureFormat::RGBA16, pipeline.raysPerProbe, totalProbes);
				}

				{
					// 1-pixel of padding surrounding each probe, 1-pixel padding surrounding entire texture for alignment.
					const int32_t irradianceWidth  = (IrradianceOctSize + 2) * uniform.probeCounts.x * uniform.probeCounts.y + 2;
					const int32_t irradianceHeight = (IrradianceOctSize + 2) * uniform.probeCounts.z + 2;
					const int32_t depthWidth       = (DepthOctSize + 2) * uniform.probeCounts.x * uniform.probeCounts.y + 2;
					const int32_t depthHeight      = (DepthOctSize + 2) * uniform.probeCounts.z + 2;

					uniform.irradianceTextureWidth  = irradianceWidth;
					uniform.irradianceTextureHeight = irradianceHeight;

					uniform.depthTextureWidth  = depthWidth;
					uniform.depthTextureHeight = depthHeight;

					for (int32_t i = 0; i < 2; i++)
					{
						internal.irradiance[i] = Texture2D::create();
						internal.depth[i]      = Texture2D::create();

						internal.depth[i]->buildTexture(TextureFormat::RG16F, depthWidth, depthHeight);
						internal.depth[i]->setName("DDGI Depth Probe Grid " + std::to_string(i));

						internal.irradiance[i]->buildTexture(TextureFormat::RGBA16, irradianceWidth, irradianceHeight);
						internal.irradiance[i]->setName("DDGI Irradiance Probe Grid " + std::to_string(i));
					}
				}

				internal.sampleProbeGrid = Texture2D::create();
				internal.sampleProbeGrid->setName("DDGI Sample Probe Grid");
				internal.sampleProbeGrid->buildTexture(TextureFormat::RGBA16, pipeline.width, pipeline.height);
			}
		}        // namespace init
	}            // namespace ddgi

	namespace trace_rays
	{
		// clang-format off
		using Entity = ecs::Registry 
			::Include<ddgi::component::Running>
			::Modify<ddgi::component::RaytracePass>
			::Modify<ddgi::component::DDGIPipelineInternal>
			::Modify<ddgi::component::DDGIPipeline>
			::Fetch<ddgi::component::DDGIUniform>
			::To<ecs::Entity>;
		// clang-format on

		inline auto system(Entity entity, component::RendererData &renderData, ecs::World world)
		{
			auto [raytracePass, internal, pipeline, uniform] = entity;
			raytracePass.samplerDescriptor->setTexture("uIrradiance", internal.irradiance[internal.pingPong]);
			raytracePass.samplerDescriptor->setTexture("uDepth", internal.depth[internal.pingPong]);

			raytracePass.sceneDescriptor->update(renderData.commandBuffer);
			raytracePass.vboDescriptor->update(renderData.commandBuffer);
			raytracePass.iboDescriptor->update(renderData.commandBuffer);
			raytracePass.materialDescriptor->update(renderData.commandBuffer);
			raytracePass.textureDescriptor->update(renderData.commandBuffer);
			raytracePass.samplerDescriptor->update(renderData.commandBuffer);
			raytracePass.outpuDescriptor->update(renderData.commandBuffer);

			raytracePass.pipeline->bind(renderData.commandBuffer);

			if (auto push = raytracePass.pipeline->getShader()->getPushConstant(0))
			{
				raytracePass.pushConsts.infiniteBounces = pipeline.infiniteBounce && !raytracePass.firstFrame ? 1 : 0;
				raytracePass.pushConsts.intensity       = pipeline.infiniteBounceIntensity;

				auto vec3 = glm::normalize(glm::vec3(internal.rand.nextReal(-1.f, 1.f),
				                                     internal.rand.nextReal(-1.f, 1.f), internal.rand.nextReal(-1.f, 1.f)));

				raytracePass.pushConsts.randomOrientation = glm::mat4_cast(
				    glm::angleAxis(internal.rand.nextReal(0.f, 1.f) * float(M_PI) * 2.0f, vec3));
				push->setData(&raytracePass.pushConsts);
			}

			raytracePass.pipeline->getShader()->bindPushConstants(renderData.commandBuffer, raytracePass.pipeline.get());
			Renderer::bindDescriptorSets(raytracePass.pipeline.get(), renderData.commandBuffer, 0, {raytracePass.sceneDescriptor, raytracePass.vboDescriptor, raytracePass.iboDescriptor, raytracePass.materialDescriptor, raytracePass.textureDescriptor, raytracePass.samplerDescriptor, raytracePass.outpuDescriptor});

			uint32_t probleCounts = uniform.probeCounts.x * uniform.probeCounts.y * uniform.probeCounts.z;
			raytracePass.pipeline->traceRays(renderData.commandBuffer, uniform.raysPerProbe, probleCounts, 1);
			raytracePass.pipeline->end(renderData.commandBuffer);
			raytracePass.firstFrame = false;
			internal.frames++;
		}
	}        // namespace trace_rays

	namespace end_frame
	{
		// clang-format off
		using Entity = ecs::Registry 
			::Include<ddgi::component::Running>
			::Modify<ddgi::component::DDGIPipelineInternal>
			::To<ecs::Entity>;
		// clang-format on

		inline auto system(Entity entity, ecs::World world)
		{
			auto [internal]   = entity;
			internal.pingPong = 1 - internal.pingPong;
		}
	}        // namespace end_frame

	namespace apply_event
	{
		// clang-format off
		using Entity = ecs::Registry 
			::Modify<ddgi::component::DDGIPipeline>
			::Modify<ddgi::component::DDGIPipelineInternal>
			::Modify<ddgi::component::DDGIUniform>
			::Include<ddgi::component::ApplyEvent>
			::Modify<ddgi::component::RaytracePass>
			::To<ecs::Entity>;
		// clang-format on

		inline auto system(Entity entity, ecs::World world)
		{
			auto [pipeline, internal, uniform, raytracePass] = entity;
			ddgi::init::initializeProbeGrid(pipeline, internal, uniform, world);
			raytracePass.outpuDescriptor->setTexture("iRadiance", internal.radiance);
			raytracePass.outpuDescriptor->setTexture("iDirectionDistance", internal.directionDepth);
			world.removeComponent<ddgi::component::ApplyEvent>(entity);
			world.addComponent<ddgi::component::Running>(entity);
		}
	}        // namespace apply_event

	namespace gather_scene
	{
		// clang-format off
		using Entity = ecs::Registry
			::Modify<ddgi::component::RaytracePass>
			::Include<ddgi::component::Running>
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

		using SkyboxGroup = ecs::Registry
			::Fetch<maple::component::SkyboxData>
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
			auto [pipeline] = entity;
			if (sceneChanged && sceneChanged->dirty && topLevels.topLevelAs)
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

				pipeline.pushConsts.numLights = lightIndicator;

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
		}
	}        // namespace gather_scene

	namespace ddgi
	{
		namespace delegates
		{
			inline auto uniformChanged(ddgi::component::DDGIPipeline &pipeline, Entity entity, ecs::World world)
			{
				auto &uniform      = entity.getComponent<component::DDGIUniform>();
				auto &bBox         = entity.getComponent<maple::component::BoundingBoxComponent>();
				auto &raytracePass = entity.getComponent<ddgi::component::RaytracePass>();

				glm::vec3 sceneLength = bBox.box->max - bBox.box->min;
				// Add 2 more probes to fully cover scene.
				uniform.probeCounts  = glm::ivec4(glm::ivec3(sceneLength / pipeline.probeDistance) + glm::ivec3(2), 1);
				uint32_t totalProbes = uniform.probeCounts.x * uniform.probeCounts.y * uniform.probeCounts.z;

				uniform.startPosition = glm::vec4(bBox.box->min, 1.f);
				uniform.step          = glm::vec4(pipeline.probeDistance);

				uniform.maxDistance    = pipeline.probeDistance * 1.5f;
				uniform.depthSharpness = pipeline.depthSharpness;
				uniform.hysteresis     = pipeline.hysteresis;
				uniform.normalBias     = pipeline.normalBias;

				uniform.energyPreservation      = pipeline.energyPreservation;
				uniform.irradianceTextureWidth  = (IrradianceOctSize + 2) * uniform.probeCounts.x * uniform.probeCounts.y + 2;
				uniform.irradianceTextureHeight = (IrradianceOctSize + 2) * uniform.probeCounts.z + 2;
				uniform.depthTextureWidth       = (DepthOctSize + 2) * uniform.probeCounts.x * uniform.probeCounts.y + 2;
				uniform.depthTextureHeight      = (DepthOctSize + 2) * uniform.probeCounts.z + 2;

				raytracePass.samplerDescriptor->setUniform("DDGIUBO", "ddgi", &uniform);
			}

			inline auto initDDGIPipeline(ddgi::component::DDGIPipeline &pipeline, Entity entity, ecs::World world)
			{
				auto &bBox = entity.addComponent<maple::component::BoundingBoxComponent>();
				auto &pipe = entity.addComponent<ddgi::component::DDGIPipelineInternal>();
				entity.addComponent<ddgi::component::DDGIUniform>();
				entity.addComponent<maple::component::Transform>();
				bBox.box = &pipe.box;

				const auto &windowSize = world.getComponent<maple::component::WindowSize>();

				float scaleDivisor = powf(2.0f, float(pipeline.scale));

				pipeline.width = windowSize.width / scaleDivisor;
				pipeline.width = windowSize.height / scaleDivisor;

				auto &raytracePass = entity.addComponent<ddgi::component::RaytracePass>();

				raytracePass.shader = Shader::create("shaders/DDGI/GIRays.shader", {{"VertexBuffer", MAX_SCENE_MESH_INSTANCE_COUNT},
				                                                                    {"IndexBuffer", MAX_SCENE_MESH_INSTANCE_COUNT},
				                                                                    {"SubmeshInfoBuffer", MAX_SCENE_MESH_INSTANCE_COUNT},
				                                                                    {"uSamplers", MAX_SCENE_MATERIAL_TEXTURE_COUNT}});
				PipelineInfo info;
				info.pipelineName     = "GIRays";
				info.shader           = raytracePass.shader;
				raytracePass.pipeline = Pipeline::get(info);

				raytracePass.lightBuffer     = StorageBuffer::create(sizeof(maple::component::LightData) * MAX_SCENE_LIGHT_COUNT, nullptr, BufferOptions{false, (int32_t) MemoryUsage::MEMORY_USAGE_CPU_TO_GPU, 0});
				raytracePass.materialBuffer  = StorageBuffer::create(sizeof(raytracing::MaterialData) * MAX_SCENE_MATERIAL_COUNT, nullptr, BufferOptions{false, (int32_t) MemoryUsage::MEMORY_USAGE_CPU_TO_GPU, 0});
				raytracePass.transformBuffer = StorageBuffer::create(sizeof(raytracing::TransformData) * MAX_SCENE_MESH_INSTANCE_COUNT, nullptr, BufferOptions{false, (int32_t) MemoryUsage::MEMORY_USAGE_CPU_TO_GPU, 0});
				raytracePass.descriptorPool  = DescriptorPool::create({25,
                                                                      {{DescriptorType::UniformBufferDynamic, 10},
                                                                       {DescriptorType::ImageSampler, MAX_SCENE_MATERIAL_TEXTURE_COUNT},
                                                                       {DescriptorType::Buffer, 5 * MAX_SCENE_MESH_INSTANCE_COUNT},
                                                                       {DescriptorType::AccelerationStructure, 10}}});

				raytracePass.sceneDescriptor    = DescriptorSet::create({0, raytracePass.shader.get(), 1, raytracePass.descriptorPool.get()});
				raytracePass.vboDescriptor      = DescriptorSet::create({1, raytracePass.shader.get(), 1, raytracePass.descriptorPool.get(), MAX_SCENE_MESH_INSTANCE_COUNT});
				raytracePass.iboDescriptor      = DescriptorSet::create({2, raytracePass.shader.get(), 1, raytracePass.descriptorPool.get(), MAX_SCENE_MESH_INSTANCE_COUNT});
				raytracePass.materialDescriptor = DescriptorSet::create({3, raytracePass.shader.get(), 1, raytracePass.descriptorPool.get(), MAX_SCENE_MESH_INSTANCE_COUNT});
				raytracePass.textureDescriptor  = DescriptorSet::create({4, raytracePass.shader.get(), 1, raytracePass.descriptorPool.get(), MAX_SCENE_MATERIAL_TEXTURE_COUNT});
				raytracePass.samplerDescriptor  = DescriptorSet::create({5, raytracePass.shader.get()});
				raytracePass.outpuDescriptor    = DescriptorSet::create({6, raytracePass.shader.get()});



				raytracePass.defaultMaterial = std::make_shared<Material>();
				MaterialProperties properties;
				properties.albedoColor       = glm::vec4(1.f, 1.f, 1.f, 1.f);
				properties.roughnessColor    = glm::vec4(0);
				properties.metallicColor     = glm::vec4(0);
				properties.usingAlbedoMap    = 0.0f;
				properties.usingRoughnessMap = 0.0f;
				properties.usingNormalMap    = 0.0f;
				properties.usingMetallicMap  = 0.0f;
				raytracePass.defaultMaterial->setMaterialProperites(properties);

				uniformChanged(pipeline, entity, world);
			}
		}        // namespace delegates

		auto registerDDGI(ExecuteQueue &begin, ExecuteQueue &render, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->onConstruct<ddgi::component::DDGIPipeline, delegates::initDDGIPipeline>();
			executePoint->onUpdate<ddgi::component::DDGIPipeline, delegates::uniformChanged>();
			executePoint->registerWithinQueue<apply_event::system>(begin);
			executePoint->registerWithinQueue<gather_scene::system>(begin);
			executePoint->registerWithinQueue<trace_rays::system>(render);
			executePoint->registerWithinQueue<end_frame::system>(render);
		}
	}        // namespace ddgi
}        // namespace maple