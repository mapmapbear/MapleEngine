//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "Voxelization.h"
#include "DrawVoxel.h"
#include "Engine/Renderer/RendererData.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/DeferredOffScreenRenderer.h"
#include "Engine/Renderer/ShadowRenderer.h"
#include "Engine/CaptureGraph.h"
#include "Engine/LPVGI/ReflectiveShadowMap.h"
#include "Engine/GBuffer.h"

#include "Scene/System/ExecutePoint.h"
#include "Scene/Component/BoundingBox.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Component/Transform.h"
#include "Scene/Component/Light.h"


#include "RHI/Texture.h"
#include "RHI/Shader.h"
#include "RHI/Pipeline.h"

#include <array>

#include "Application.h"

namespace maple
{
	enum DescriptorID
	{
		VertexUniform,
		MaterialBinding,//default id for material is 1..
		FragmentUniform,
		GeometryUniform,
		DescriptorLength
	};

	namespace vxgi
	{
		namespace global
		{
			namespace component
			{
				struct VoxelRadianceInjectionPipline
				{
					std::shared_ptr<Shader> shader;
					std::vector<std::shared_ptr<DescriptorSet>> descriptors;

					struct UniformBufferVX
					{
						glm::vec4 worldMinPoint;//use xyz
						float voxelSize;
						float voxelScale;
						float traceShadowHit = 0.5;
						int32_t volumeDimension = vxgi::component::Voxelization::voxelDimension;
					}uniformData;
				};

				struct VoxelMipmapVolumePipline
				{
					std::shared_ptr<Shader> shader;
					std::vector<std::shared_ptr<DescriptorSet>> descriptors;
				};

				struct VoxelMipmapBasePipline
				{
					std::shared_ptr<Shader> shader;
					std::vector<std::shared_ptr<DescriptorSet>> descriptors;
				};

				struct VoxelRadiancePropagationPipline
				{
					std::shared_ptr<Shader> shader;
					std::vector<std::shared_ptr<DescriptorSet>> descriptors;
				};

				struct IndirectLightPipeline
				{
					std::shared_ptr<Shader> shader;
					std::vector<std::shared_ptr<DescriptorSet>> descriptors;
				};
			}
		}

		namespace
		{
			inline auto initVoxelBuffer(global::component::VoxelBuffer& buffer)
			{
				for (int32_t i = 0; i < VoxelBufferId::Length; i++)
				{
					buffer.voxelVolume[i] = Texture3D::create(
						component::Voxelization::voxelDimension,
						component::Voxelization::voxelDimension,
						component::Voxelization::voxelDimension,
						{ TextureFormat::R32UI, TextureFilter::Nearest,TextureWrap::ClampToEdge},
						{ false,false,false,true }
					);

					buffer.voxelVolume[i]->setName(VoxelBufferId::Names[i]);
				}

				for (int32_t i = 0; i < 6; i++)
				{
					buffer.voxelTexMipmap[i] = Texture3D::create(
						component::Voxelization::voxelDimension / 2,
						component::Voxelization::voxelDimension / 2,
						component::Voxelization::voxelDimension / 2,
						{ TextureFormat::RGBA8, TextureFilter::Linear,TextureWrap::ClampToEdge },
						{ false,false,true }
					);
				}

				buffer.staticFlag = Texture3D::create(
					component::Voxelization::voxelDimension,
					component::Voxelization::voxelDimension,
					component::Voxelization::voxelDimension,
					{ TextureFormat::R8, TextureFilter::Nearest,TextureWrap::ClampToEdge },
					{ false,false,false }
				);

				buffer.colorBuffer = Texture2D::create();
				buffer.colorBuffer->buildTexture(TextureFormat::RGBA8, 256, 256);

				buffer.voxelShader = Shader::create("shaders/VXGI/Voxelization.shader");
				buffer.descriptors.resize(DescriptorLength);
				for (uint32_t i = 0; i < DescriptorLength; i++)
				{
					if (i != MaterialBinding)
						buffer.descriptors[i] = DescriptorSet::create({ i,buffer.voxelShader.get() });
				}
			}

			using LightDefine = ecs::Chain
				::Write<maple::component::Light>;

			using LightEntity = LightDefine
				::To<ecs::Entity>;

			using LightQuery = LightDefine
				::To<ecs::Query>;

			inline auto updateLightingBuffer(
				const vxgi::component::Voxelization& voxel,
				LightQuery lightQuery,
				global::component::VoxelBuffer& buffer,
				maple::component::ShadowMapData& shadowMapData,
				maple::component::CameraView& cameraView,
				global::component::VoxelRadianceInjectionPipline& injection,
				const maple::component::RendererData& rendererData
			)
			{
				maple::component::LightData lights[32] = {};
				uint32_t numLights = 0;

				lightQuery.forEach([&](LightEntity entity) {
					auto [light] = entity;
					lights[numLights] = light.lightData;
					numLights++;
					});

				injection.descriptors[0]->setUniform("UniformBufferLight", "lights", lights, sizeof(maple::component::LightData) * numLights, false);
				injection.descriptors[0]->setUniform("UniformBufferLight", "lightCount", &numLights);

				injection.uniformData.traceShadowHit = voxel.traceShadowHit;

				injection.descriptors[0]->setUniformBufferData("UniformBufferVX", &injection.uniformData);

				injection.descriptors[0]->setTexture("uVoxelAlbedo", buffer.voxelVolume[VoxelBufferId::Albedo]);
				injection.descriptors[0]->setTexture("uVoxelNormal", buffer.voxelVolume[VoxelBufferId::Normal]);
				injection.descriptors[0]->setTexture("uVoxelRadiance", buffer.voxelVolume[VoxelBufferId::Radiance]);
				injection.descriptors[0]->setTexture("uVoxelEmissive", buffer.voxelVolume[VoxelBufferId::Emissive]);

				injection.descriptors[0]->update(rendererData.computeCommandBuffer);
			}
		}

		namespace begin_scene
		{
			using Entity = ecs::Chain
				::Write<component::Voxelization>
				::To<ecs::Entity>;

			using Query = ecs::Chain
				::Write<maple::component::MeshRenderer>
				::Write<maple::component::Transform>
				::To<ecs::Query>;

			inline auto system(Entity entity,
				Query meshQuery,
				LightQuery lightQuery,
				global::component::VoxelBuffer& buffer,
				global::component::VoxelRadianceInjectionPipline& injection,
				maple::component::ShadowMapData& shadowMapData,
				maple::component::CameraView& cameraView,
				maple::component::BoundingBoxComponent& aabb,
				const maple::component::RendererData& renderData,
				ecs::World world)
			{
				auto [voxel] = entity;

				if (!voxel.dirty)
					return;

				buffer.commandQueue.clear();

				for (auto entityHandle : meshQuery)
				{
					auto [mesh, trans] = meshQuery.convert(entityHandle);
					{
						const auto& worldTransform = trans.getWorldMatrix();
						if (!mesh.active)
							return;

						auto& cmd = buffer.commandQueue.emplace_back();
						cmd.mesh = mesh.mesh.get();
						cmd.transform = worldTransform;

						if (mesh.mesh->getSubMeshCount() <= 1)
						{
							cmd.material = mesh.mesh->getMaterial().empty() ? nullptr : mesh.mesh->getMaterial()[0].get();
						}
					}
				}

				updateLightingBuffer(voxel, lightQuery, buffer, shadowMapData, cameraView, injection, renderData);
			}
		}

		namespace listen_light_change
		{
			using Entity = ecs::Chain
				::Without<component::UpdateRadiance>
				::Write<component::Voxelization>
				::To<ecs::Entity>;

			inline auto system(Entity entity,
				LightQuery lightQuery,
				global::component::VoxelBuffer& buffer,
				global::component::VoxelRadianceInjectionPipline& injection,
				const maple::global::component::SceneTransformChanged& changed,
				maple::component::ShadowMapData& shadowMapData,
				maple::component::CameraView& cameraView,
				const maple::component::RendererData& renderData,
				ecs::World world)
			{

				auto [voxel] = entity;

				for (auto ent : changed.entities)
				{
					if (lightQuery.contains(ent))
					{
						updateLightingBuffer(voxel, lightQuery, buffer, shadowMapData, cameraView, injection, renderData);
						world.addComponent<component::UpdateRadiance>(entity);
						break;
					}
				}
			}
		}

		namespace update_radiance
		{
			using Entity = ecs::Chain
				::Check<component::UpdateRadiance>
				::Write<component::Voxelization>
				::To<ecs::Entity>;

			using Query = ecs::Chain
				::Read<maple::component::Light>
				::To<ecs::Query>;

			/**
			 * voxelTexMipmap radiance -> mipmap
			 */
			inline auto generateMipmapVolume(
				global::component::VoxelBuffer& voxelBuffer,
				global::component::VoxelMipmapVolumePipline& volumePipline,
				maple::component::BoundingBoxComponent& box,
				const maple::component::RendererData& renderData)
			{
				auto mipmapDim = vxgi::component::Voxelization::voxelDimension / 4;

				for (auto mipLvl = 0; mipmapDim >= 1; mipmapDim /= 2, mipLvl++)
				{
					auto volumeSize = glm::vec3(mipmapDim, mipmapDim, mipmapDim);

					volumePipline.descriptors[mipLvl]->setUniform("UniformBufferObject", "mipDimension", &volumeSize);
					volumePipline.descriptors[mipLvl]->setUniform("UniformBufferObject", "mipLevel", &mipLvl);
					volumePipline.descriptors[mipLvl]->setTexture("uVoxelMipmapIn",
						{ voxelBuffer.voxelTexMipmap.begin(), voxelBuffer.voxelTexMipmap.end() }
					);
					volumePipline.descriptors[mipLvl]->setTexture("uVoxelMipmapOut",
						{ voxelBuffer.voxelTexMipmap.begin(),voxelBuffer.voxelTexMipmap.end() },
						mipLvl + 1
					);
					volumePipline.descriptors[mipLvl]->update(renderData.computeCommandBuffer);

					PipelineInfo pipelineInfo;
					pipelineInfo.shader = volumePipline.shader;
					pipelineInfo.groupCountX = static_cast<uint32_t>(glm::ceil(mipmapDim / (float)volumePipline.shader->getLocalSizeX()));
					pipelineInfo.groupCountY = static_cast<uint32_t>(glm::ceil(mipmapDim / (float)volumePipline.shader->getLocalSizeY()));
					pipelineInfo.groupCountZ = static_cast<uint32_t>(glm::ceil(mipmapDim / (float)volumePipline.shader->getLocalSizeZ()));

					auto pipeline = Pipeline::get(pipelineInfo);
					pipeline->bind(renderData.computeCommandBuffer);
					Renderer::bindDescriptorSets(pipeline.get(), renderData.computeCommandBuffer, 0, { volumePipline.descriptors[mipLvl] });
					Renderer::dispatch(
						renderData.computeCommandBuffer,
						pipelineInfo.groupCountX,
						pipelineInfo.groupCountY,
						pipelineInfo.groupCountZ
					);

					for (auto& map : voxelBuffer.voxelTexMipmap)
					{
						map->memoryBarrier(renderData.computeCommandBuffer,
							MemoryBarrierFlags::Shader_Image_Access_Barrier
						);
					}

					pipeline->end(renderData.computeCommandBuffer);
				}
			}

			inline auto generateMipmapMipmap(
				const std::shared_ptr<Texture3D>& voxelRadiance,
				const maple::component::RendererData& renderData,
				global::component::VoxelBuffer& buffer,
				global::component::VoxelMipmapBasePipline& pipline)
			{
				auto halfDimension = component::Voxelization::voxelDimension / 2; //dimension for voxelMipmap
				pipline.descriptors[0]->setUniform("UniformBufferObject", "mipDimension", &halfDimension);
				pipline.descriptors[0]->setTexture("uVoxelMipmap", { buffer.voxelTexMipmap.begin(),buffer.voxelTexMipmap.end() });
				pipline.descriptors[0]->setTexture("uVoxelBase", voxelRadiance);
				pipline.descriptors[0]->update(renderData.computeCommandBuffer);


				PipelineInfo pipelineInfo;
				pipelineInfo.shader = pipline.shader;

				pipelineInfo.groupCountX = halfDimension / pipline.shader->getLocalSizeX();
				pipelineInfo.groupCountY = halfDimension / pipline.shader->getLocalSizeY();
				pipelineInfo.groupCountZ = halfDimension / pipline.shader->getLocalSizeZ();

				auto pipeline = Pipeline::get(pipelineInfo);
				pipeline->bind(renderData.computeCommandBuffer);
				Renderer::bindDescriptorSets(pipeline.get(), renderData.computeCommandBuffer, 0, pipline.descriptors);
				Renderer::dispatch(
					renderData.computeCommandBuffer,
					pipelineInfo.groupCountX,
					pipelineInfo.groupCountY,
					pipelineInfo.groupCountZ
				);
	
				voxelRadiance->memoryBarrier(renderData.computeCommandBuffer,
					MemoryBarrierFlags::Shader_Image_Access_Barrier
				);

				for (auto & map : buffer.voxelTexMipmap)
				{
					map->memoryBarrier(renderData.computeCommandBuffer,
						MemoryBarrierFlags::Shader_Image_Access_Barrier
					);
				}

				pipeline->end(renderData.computeCommandBuffer);
			}

			inline auto system(
				Entity entity,
				Query lightQuery,
				global::component::VoxelBuffer& buffer,
				global::component::VoxelRadianceInjectionPipline& injection,
				global::component::VoxelMipmapBasePipline& basePipeline,
				global::component::VoxelMipmapVolumePipline& volumePipline,
				global::component::VoxelRadiancePropagationPipline& propagation,
				maple::component::BoundingBoxComponent& box,
				const maple::component::RendererData& renderData,
				ecs::World world)
			{

				auto [voxel] = entity;

				auto hasUpdateRadiance = entity.hasComponent<component::UpdateRadiance>();

				if (!voxel.dirty && !hasUpdateRadiance)
					return;

				buffer.voxelVolume[VoxelBufferId::Radiance]->clear(renderData.computeCommandBuffer);

				PipelineInfo pipelineInfo;
				pipelineInfo.shader = injection.shader;

				pipelineInfo.groupCountX = component::Voxelization::voxelDimension / injection.shader->getLocalSizeX();
				pipelineInfo.groupCountY = component::Voxelization::voxelDimension / injection.shader->getLocalSizeY();
				pipelineInfo.groupCountZ = component::Voxelization::voxelDimension / injection.shader->getLocalSizeZ();

				auto pipeline = Pipeline::get(pipelineInfo);
				pipeline->bind(renderData.computeCommandBuffer);
				Renderer::bindDescriptorSets(pipeline.get(), renderData.computeCommandBuffer, 0, injection.descriptors);
				Renderer::dispatch(
					renderData.computeCommandBuffer,
					pipelineInfo.groupCountX,
					pipelineInfo.groupCountY,
					pipelineInfo.groupCountZ
				);

				buffer.voxelVolume[VoxelBufferId::Radiance]->memoryBarrier(renderData.computeCommandBuffer, MemoryBarrierFlags::Shader_Storage_Barrier);
				buffer.voxelVolume[VoxelBufferId::Emissive]->memoryBarrier(renderData.computeCommandBuffer, MemoryBarrierFlags::Shader_Image_Access_Barrier);
				buffer.voxelVolume[VoxelBufferId::Albedo]->memoryBarrier(renderData.computeCommandBuffer, MemoryBarrierFlags::Texture_Fetch_Barrier);
				buffer.voxelVolume[VoxelBufferId::Normal]->memoryBarrier(renderData.computeCommandBuffer, MemoryBarrierFlags::Shader_Storage_Barrier);

				pipeline->end(renderData.computeCommandBuffer);

				generateMipmapMipmap(buffer.voxelVolume[VoxelBufferId::Radiance], renderData, buffer, basePipeline);
				generateMipmapVolume(buffer, volumePipline, box, renderData);

				if (voxel.injectFirstBounce)
				{
					//propagation .............
					propagation.descriptors[0]->setUniform("UniformBufferObject", "maxTracingDistanceGlobal", &voxel.maxTracingDistance);
					propagation.descriptors[0]->setUniform("UniformBufferObject", "volumeDimension", &component::Voxelization::voxelDimension);
					propagation.descriptors[0]->setTexture("uVoxelComposite", buffer.voxelVolume[VoxelBufferId::Radiance]);
					propagation.descriptors[0]->setTexture("uVoxelAlbedo", buffer.voxelVolume[VoxelBufferId::Albedo]);
					propagation.descriptors[0]->setTexture("uVoxelNormal", buffer.voxelVolume[VoxelBufferId::Normal]);
					propagation.descriptors[0]->setTexture("uVoxelTexMipmap", { buffer.voxelTexMipmap.begin(), buffer.voxelTexMipmap.end() });
					propagation.descriptors[0]->update(renderData.computeCommandBuffer);

					PipelineInfo pipelineInfo;
					pipelineInfo.shader = propagation.shader;
					pipelineInfo.groupCountX = component::Voxelization::voxelDimension / propagation.shader->getLocalSizeX();
					pipelineInfo.groupCountY = component::Voxelization::voxelDimension / propagation.shader->getLocalSizeY();
					pipelineInfo.groupCountZ = component::Voxelization::voxelDimension / propagation.shader->getLocalSizeZ();

					auto pipeline = Pipeline::get(pipelineInfo);
					pipeline->bind(renderData.computeCommandBuffer);
					Renderer::bindDescriptorSets(pipeline.get(), renderData.computeCommandBuffer, 0, propagation.descriptors);
					Renderer::dispatch(
						renderData.computeCommandBuffer,
						pipelineInfo.groupCountX,
						pipelineInfo.groupCountY,
						pipelineInfo.groupCountZ
					);
				

					buffer.voxelVolume[VoxelBufferId::Radiance]->memoryBarrier(renderData.computeCommandBuffer, 
						MemoryBarrierFlags::Shader_Storage_Barrier | 
						MemoryBarrierFlags::Shader_Image_Access_Barrier
					);
					buffer.voxelVolume[VoxelBufferId::Albedo]->memoryBarrier(renderData.computeCommandBuffer, MemoryBarrierFlags::Texture_Fetch_Barrier);
					buffer.voxelVolume[VoxelBufferId::Normal]->memoryBarrier(renderData.computeCommandBuffer, MemoryBarrierFlags::Texture_Fetch_Barrier);

				

					pipeline->end(renderData.computeCommandBuffer);

					generateMipmapMipmap(buffer.voxelVolume[VoxelBufferId::Radiance], renderData, buffer, basePipeline);
					generateMipmapVolume(buffer, volumePipline, box, renderData);
				}

				Application::getRenderDoc().endCapture();

				if (hasUpdateRadiance)
					world.removeComponent<component::UpdateRadiance>(entity);

				voxel.dirty = false;
			}
		}

		namespace voxelize_dynamic_scene
		{
			using Entity = ecs::Chain
				::Write<component::Voxelization>
				::To<ecs::Entity>;

			inline auto system(Entity entity, const maple::component::RendererData& renderData, ecs::World world)
			{

			}
		}

		namespace voxelize_static_scene
		{
			using Entity = ecs::Chain
				::Write<component::Voxelization>
				::To<ecs::Entity>;

			inline auto system(Entity entity,
				global::component::VoxelBuffer& buffer,
				maple::component::CameraView& cameraView,
				maple::component::RendererData& renderData,
				maple::component::DeferredData& deferredData,
				capture_graph::component::RenderGraph& graph,
				ecs::World world)
			{

				auto [voxel] = entity;
				if (!voxel.dirty)
					return;

				for (auto text : buffer.voxelVolume)
				{
					text->clear(renderData.commandBuffer);
				}
				//voxelize the whole scene now. this would be optimize in the future.
				//voxelization the inner room is a good choice.
				buffer.descriptors[DescriptorID::VertexUniform]->setUniform("UniformBufferObjectVert", "projView", &cameraView.projView);
				buffer.descriptors[DescriptorID::VertexUniform]->update(renderData.commandBuffer);

				buffer.descriptors[DescriptorID::FragmentUniform]->setTexture("uVoxelAlbedo", buffer.voxelVolume[VoxelBufferId::Albedo]);
				buffer.descriptors[DescriptorID::FragmentUniform]->setTexture("uVoxelNormal", buffer.voxelVolume[VoxelBufferId::Normal]);
				buffer.descriptors[DescriptorID::FragmentUniform]->setTexture("uVoxelEmission", buffer.voxelVolume[VoxelBufferId::Emissive]);
				buffer.descriptors[DescriptorID::FragmentUniform]->setTexture("uStaticVoxelFlag", buffer.staticFlag);
				buffer.descriptors[DescriptorID::FragmentUniform]->update(renderData.commandBuffer);


				for (auto texture : buffer.voxelVolume)
				{
					LOGI("{:x}", texture->toIntID());
				}

				PipelineInfo pipeInfo;
				pipeInfo.shader = buffer.voxelShader;
				pipeInfo.cullMode = CullMode::None;
				pipeInfo.depthTest = false;
				pipeInfo.clearTargets = true;
				pipeInfo.colorTargets[0] = buffer.colorBuffer;

				auto pipeline = Pipeline::get(pipeInfo, buffer.descriptors, graph);
				pipeline->bind(renderData.commandBuffer);

				for (auto& cmd : deferredData.commandQueue)
				{
					cmd.material->setShader(buffer.voxelShader);
					cmd.material->bind(renderData.commandBuffer);
					buffer.descriptors[DescriptorID::MaterialBinding] = cmd.material->getDescriptorSet();

					auto& pushConstants = buffer.voxelShader->getPushConstants()[0];

					pushConstants.setValue("transform", &cmd.transform);
					buffer.voxelShader->bindPushConstants(renderData.commandBuffer, pipeline.get());

					Renderer::bindDescriptorSets(pipeline.get(), renderData.commandBuffer, 0, buffer.descriptors);
					Renderer::drawMesh(renderData.commandBuffer, pipeline.get(), cmd.mesh);
				}

				buffer.voxelVolume[VoxelBufferId::Albedo]->memoryBarrier(renderData.commandBuffer, MemoryBarrierFlags::Shader_Storage_Barrier);
				buffer.voxelVolume[VoxelBufferId::Normal]->memoryBarrier(renderData.commandBuffer, MemoryBarrierFlags::Shader_Storage_Barrier);
				buffer.voxelVolume[VoxelBufferId::Emissive]->memoryBarrier(renderData.commandBuffer, MemoryBarrierFlags::Shader_Storage_Barrier);
				buffer.staticFlag->memoryBarrier(renderData.commandBuffer, MemoryBarrierFlags::Shader_Storage_Barrier);

				pipeline->end(renderData.commandBuffer);
				voxel.dirty = false;

				Application::getRenderDoc().endCapture();


			}//next is dynamic
		}

		namespace setup_voxel_volumes
		{
			using Entity = ecs::Chain
				::Write<component::Voxelization>
				::To<ecs::Entity>;

			inline auto system(Entity entity,
				global::component::VoxelBuffer& voxelBuffer,
				global::component::VoxelRadianceInjectionPipline& injection,
				maple::component::BoundingBoxComponent& box,
				const maple::component::RendererData& rendererData,
				ecs::World world)
			{
				auto [voxelization] = entity;
				if (box.box)
				{
					if (voxelBuffer.box != *box.box)
					{
						Application::getRenderDoc().startCapture();

						voxelBuffer.box = *box.box;

						auto axisSize = voxelBuffer.box.size();
						auto center = voxelBuffer.box.center();
						voxelization.volumeGridSize = glm::max(axisSize.x, glm::max(axisSize.y, axisSize.z));
						voxelization.voxelSize = voxelization.volumeGridSize / component::Voxelization::voxelDimension;

						injection.uniformData.volumeDimension = component::Voxelization::voxelDimension;
						injection.uniformData.worldMinPoint = { box.box->min ,1.f };
						injection.uniformData.voxelSize = voxelization.voxelSize;
						injection.uniformData.voxelScale = 1 / voxelization.volumeGridSize;

						auto halfSize = voxelization.volumeGridSize / 2.0f;
						// projection matrix
						auto projection = glm::ortho(-halfSize, halfSize, -halfSize, halfSize, 0.0f, voxelization.volumeGridSize);
						// view matrices
						voxelBuffer.viewProj[0] = lookAt(center + glm::vec3(halfSize, 0.0f, 0.0f),
							center, glm::vec3(0.0f, 1.0f, 0.0f));
						voxelBuffer.viewProj[1] = lookAt(center + glm::vec3(0.0f, halfSize, 0.0f),
							center, glm::vec3(0.0f, 0.0f, -1.0f));
						voxelBuffer.viewProj[2] = lookAt(center + glm::vec3(0.0f, 0.0f, halfSize),
							center, glm::vec3(0.0f, 1.0f, 0.0f));

						int32_t i = 0;

						for (auto& matrix : voxelBuffer.viewProj)
						{
							matrix = projection * matrix;
							voxelBuffer.viewProjInverse[i++] = glm::inverse(matrix);
						}

						const float voxelScale = 1.f / voxelization.volumeGridSize;
						const float dimension = component::Voxelization::voxelDimension;

						voxelBuffer.descriptors[DescriptorID::GeometryUniform]->setUniform("UniformBufferGemo", "viewProjections", &voxelBuffer.viewProj);
						voxelBuffer.descriptors[DescriptorID::GeometryUniform]->setUniform("UniformBufferGemo", "viewProjectionsI", &voxelBuffer.viewProjInverse);
						voxelBuffer.descriptors[DescriptorID::GeometryUniform]->setUniform("UniformBufferGemo", "worldMinPoint", &box.box->min);
						voxelBuffer.descriptors[DescriptorID::GeometryUniform]->setUniform("UniformBufferGemo", "voxelScale", &voxelScale);
						voxelBuffer.descriptors[DescriptorID::GeometryUniform]->setUniform("UniformBufferGemo", "volumeDimension", &dimension);
						voxelBuffer.descriptors[DescriptorID::GeometryUniform]->update(rendererData.commandBuffer);

						uint32_t flagVoxel = 1;
						voxelBuffer.descriptors[DescriptorID::FragmentUniform]->setUniform("UniformBufferObject", "flagStaticVoxels", &flagVoxel);
						voxelBuffer.descriptors[DescriptorID::FragmentUniform]->update(rendererData.commandBuffer);

						voxelization.dirty = true;
					}
				}
			}
		}

		namespace indirect_lighting_setup
		{
			using Entity = ecs::Chain
				::Read<vxgi::component::Voxelization>
				::To<ecs::Entity>;

			inline auto system(Entity entity, 
				const vxgi::global::component::VoxelBuffer& vxgiBuffer, 
				const maple::component::CameraView & cameraView,
				const maple::component::RendererData& rendererData,
				global::component::IndirectLightPipeline& pipeline)
			{
				auto [vxgi] = entity;

				float scale = 1.f / vxgi.volumeGridSize;
				pipeline.descriptors[0]->setUniform("UniformBufferVXGI", "cameraPosition", glm::value_ptr(glm::vec4(cameraView.cameraTransform->getWorldPosition(),1.f)));
				pipeline.descriptors[0]->setUniform("UniformBufferVXGI", "volumeDimension", &vxgi::component::Voxelization::voxelDimension);
				pipeline.descriptors[0]->setUniform("UniformBufferVXGI", "voxelScale", &scale);
				pipeline.descriptors[0]->setUniform("UniformBufferVXGI", "worldSize", &vxgi.volumeGridSize);
				pipeline.descriptors[0]->setUniform("UniformBufferVXGI", "maxTracingDistanceGlobal", &vxgi.maxTracingDistance);
				pipeline.descriptors[0]->setUniform("UniformBufferVXGI", "aoFalloff", &vxgi.aoFalloff);
				pipeline.descriptors[0]->setUniform("UniformBufferVXGI", "aoAlpha", &vxgi.aoAlpha);
				pipeline.descriptors[0]->setUniform("UniformBufferVXGI", "bounceStrength", &vxgi.bounceStrength);
				pipeline.descriptors[0]->setUniform("UniformBufferVXGI", "samplingFactor", &vxgi.samplingFactor);
				pipeline.descriptors[0]->setUniform("UniformBufferVXGI", "worldMinPoint", &vxgiBuffer.box.min);
				pipeline.descriptors[0]->setUniform("UniformBufferVXGI", "worldMaxPoint", &vxgiBuffer.box.max);
				pipeline.descriptors[0]->setTexture("uVoxelTex", vxgiBuffer.voxelVolume[VoxelBufferId::Radiance]);
				pipeline.descriptors[0]->setTexture("uVoxelTexMipmap", { vxgiBuffer.voxelTexMipmap.begin(), vxgiBuffer.voxelTexMipmap.end() });
				pipeline.descriptors[0]->setTexture("uIndirectLight", rendererData.gbuffer->getBuffer(GBufferTextures::INDIRECT_LIGHTING));
				pipeline.descriptors[0]->setTexture("uColorSampler", rendererData.gbuffer->getBuffer(GBufferTextures::COLOR));
				pipeline.descriptors[0]->setTexture("uPositionSampler", rendererData.gbuffer->getBuffer(GBufferTextures::POSITION));
				pipeline.descriptors[0]->setTexture("uNormalSampler", rendererData.gbuffer->getBuffer(GBufferTextures::NORMALS));
				pipeline.descriptors[0]->setTexture("uPBRSampler", rendererData.gbuffer->getBuffer(GBufferTextures::PBR));
				pipeline.descriptors[0]->update(rendererData.computeCommandBuffer);
			}
		}

		namespace compute_indirect_light
		{
			using Entity = ecs::Chain
				::Write<component::Voxelization>
				::To<ecs::Entity>;

			inline auto system(Entity entity,
				const global::component::VoxelBuffer& voxelBuffer,
				global::component::IndirectLightPipeline & indirectPipeline,
				maple::component::BoundingBoxComponent& box,
				const maple::component::RendererData& rendererData,
				const maple::component::WindowSize & winSize,
				ecs::World world)
			{
				auto [voxelization] = entity;

				PipelineInfo pipelineInfo;
				pipelineInfo.shader = indirectPipeline.shader;
				pipelineInfo.groupCountX = winSize.width / indirectPipeline.shader->getLocalSizeX();
				pipelineInfo.groupCountY = winSize.height / indirectPipeline.shader->getLocalSizeY();

				auto pipeline = Pipeline::get(pipelineInfo);
				pipeline->bind(rendererData.computeCommandBuffer);
				Renderer::bindDescriptorSets(pipeline.get(), rendererData.computeCommandBuffer, 0, indirectPipeline.descriptors);
				Renderer::dispatch(
					rendererData.computeCommandBuffer,
					pipelineInfo.groupCountX,
					pipelineInfo.groupCountY,
					1
				);
				pipeline->end(rendererData.computeCommandBuffer);
			}
		}

		auto registerGlobalComponent(std::shared_ptr<ExecutePoint> point) -> void
		{
			point->registerGlobalComponent<global::component::VoxelBuffer>(&initVoxelBuffer);

			point->registerGlobalComponent<global::component::VoxelRadianceInjectionPipline>([](auto& injection) {
				injection.shader = Shader::create("shaders/VXGI/InjectRadiance.shader");
				injection.descriptors.emplace_back(
					DescriptorSet::create({ 0, injection.shader.get() })
				);
				});

			point->registerGlobalComponent<global::component::VoxelMipmapVolumePipline>([](auto& pipline) {
				pipline.shader = Shader::create("shaders/VXGI/AnisoMipmapVolume.shader");
				
				auto mipmapDim = vxgi::component::Voxelization::voxelDimension / 4;

				for (auto mipLvl = 0; mipmapDim >= 1; mipmapDim /= 2, mipLvl++)
				{
					pipline.descriptors.emplace_back(
						DescriptorSet::create({ 0, pipline.shader.get() })
					);
				}
			});

			point->registerGlobalComponent<global::component::VoxelMipmapBasePipline>([](auto& pipline) {
				pipline.shader = Shader::create("shaders/VXGI/AnisoMipmapBase.shader");
				pipline.descriptors.emplace_back(
					DescriptorSet::create({ 0, pipline.shader.get() })
				);
				});

			point->registerGlobalComponent<global::component::VoxelRadiancePropagationPipline>([](auto& pipline) {
				pipline.shader = Shader::create("shaders/VXGI/PropagationRadiance.shader");
				pipline.descriptors.emplace_back(
					DescriptorSet::create({ 0, pipline.shader.get() })
				);
				});

			point->registerGlobalComponent<global::component::IndirectLightPipeline>([](auto & pipeline) {
				pipeline.shader = Shader::create("shaders/VXGI/IndirectLight.shader");
				pipeline.descriptors.emplace_back(
					DescriptorSet::create({ 0, pipeline.shader.get() })
				);
			});
		}

		auto registerVoxelizer(ExecuteQueue& begin, ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> point) -> void
		{
			point->registerWithinQueue<setup_voxel_volumes::system>(begin);
			point->registerWithinQueue<begin_scene::system>(begin);
			point->registerWithinQueue<indirect_lighting_setup::system>(begin);
			point->registerWithinQueue<listen_light_change::system>(begin);

			point->registerWithinQueue<voxelize_static_scene::system>(renderer);
			//point->registerWithinQueue<voxelize_dynamic_scene::system>(renderer);
			//point->registerWithinQueue<update_radiance::system>(renderer);
		}

		auto registerVXGIIndirectLighting(ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> point) -> void
		{
			//point->registerWithinQueue<compute_indirect_light::system>(renderer);
		}
	}
};