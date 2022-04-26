//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "Voxelization.h"
#include "Engine/Renderer/RendererData.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/DeferredOffScreenRenderer.h"
#include "Engine/CaptureGraph.h"
#include "Engine/Vientiane/ReflectiveShadowMap.h"

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
	enum VoxelBufferId
	{
		Albedo,
		Normal,
		Radiance,
		Emissive,
		Length
	};

	enum DescriptorID
	{
		VertexUniform,
		MaterialBinding,//default id for material is 1..
		FragmentUniform,
		GeometryUniform,
		DescriptorLength
	};

	namespace global
	{
		namespace component 
		{
			struct VoxelBuffer
			{
				std::array<glm::mat4, 3> viewProj;
				std::array<glm::mat4, 3> viewProjInverse;
				BoundingBox box;
				float volumeGridSize;
				float voxelSize;

				std::array<	std::shared_ptr<Texture3D>, VoxelBufferId::Length> voxelVolume;
				std::array<	std::shared_ptr<Texture3D>, 6> voxelTexMipmap;
				std::shared_ptr<Texture3D> staticFlag;

				std::shared_ptr<Shader> voxelShader;
				std::vector<std::shared_ptr<DescriptorSet>> descriptors;

				std::vector<RenderCommand> commandQueue;

				std::shared_ptr<Texture2D> colorBuffer;
			};

			struct VoxelRadianceInjection 
			{
				std::shared_ptr<Shader> shader;
				std::vector<std::shared_ptr<DescriptorSet>> descriptors;

				struct UniformBufferVX 
				{
					glm::vec4 exponents;//use x/y
					glm::vec4 worldMinPoint;//use xyz
					float lightBleedingReduction = 0.f;
					float voxelSize;
					float voxelScale;
					float traceShadowHit = 0.5;
					int32_t volumeDimension = maple::component::Voxelization::voxelDimension;
					int32_t normalWeightedLambert = 1;//default is 1
					int32_t shadowingMethod = 2;//2 trace / 1 shadow-mapping
					int32_t padding;
				}uniformData;
			};
		}
	}

	namespace 
	{
		inline auto initVoxelBuffer(global::component::VoxelBuffer & buffer)
		{
			for (int32_t i = 0; i < VoxelBufferId::Length; i++)
			{
				buffer.voxelVolume[i] = Texture3D::create(
					component::Voxelization::voxelDimension,
					component::Voxelization::voxelDimension,
					component::Voxelization::voxelDimension,
					{ TextureFormat::R32UI, TextureFilter::Linear,TextureWrap::ClampToEdge },
					{ false,false,false }
				);
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
			for (uint32_t i = 0;i< DescriptorLength;i++)
			{
				if(i != MaterialBinding)
					buffer.descriptors[i] = DescriptorSet::create({ i,buffer.voxelShader.get() });
			}
		}
	}

	namespace vxgi
	{
		namespace begin_scene 
		{
			using Entity = ecs::Chain
				::Write<component::Voxelization>
				::To<ecs::Entity>;

			using Query = ecs::Chain
				::Write<component::MeshRenderer>
				::Write<component::Transform>
				::To<ecs::Query>;

			using LightDefine = ecs::Chain
				::Write<component::Light>;

			using LightEntity = LightDefine
				::To<ecs::Entity>;

			using LightQuery = LightDefine
				::To<ecs::Query>;

			inline auto system(Entity entity, 
				Query meshQuery, 
				LightQuery lightQuery,
				global::component::VoxelBuffer & buffer, 
				global::component::VoxelRadianceInjection& injection,
				component::ShadowMapData & shadowMapData,
				component::CameraView & cameraView,
				component::BoundingBoxComponent& aabb,
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

				component::LightData lights[32] = {};
				uint32_t numLights = 0;

				lightQuery.forEach([&](LightEntity entity) {
					auto [light] = entity;
					lights[numLights] = light.lightData;
					numLights++;
				});

		
				injection.descriptors[0]->setUniform("UniformBufferLight", "lights", lights, sizeof(component::LightData) * numLights, false);
				injection.descriptors[0]->setUniform("UniformBufferLight", "viewMatrix", &cameraView.view);
				injection.descriptors[0]->setUniform("UniformBufferLight", "lightView", &shadowMapData.lightMatrix);
				injection.descriptors[0]->setUniform("UniformBufferLight", "shadowCount", &shadowMapData.shadowMapNum);
				injection.descriptors[0]->setUniform("UniformBufferLight", "shadowTransform", shadowMapData.shadowProjView);
				injection.descriptors[0]->setUniform("UniformBufferLight", "splitDepths", shadowMapData.splitDepth);
				injection.descriptors[0]->setUniform("UniformBufferLight", "biasMat", &BIAS_MATRIX);
				injection.descriptors[0]->setUniform("UniformBufferLight", "initialBias", &shadowMapData.initialBias);

				injection.descriptors[0]->setUniformBufferData("UniformBufferVX", &injection.uniformData);

				injection.descriptors[0]->setTexture("uVoxelAlbedo", buffer.voxelVolume[VoxelBufferId::Albedo]);
				injection.descriptors[0]->setTexture("uVoxelNormal", buffer.voxelVolume[VoxelBufferId::Normal]);
				injection.descriptors[0]->setTexture("uVoxelRadiance", buffer.voxelVolume[VoxelBufferId::Radiance]);
				injection.descriptors[0]->setTexture("uVoxelEmissive", buffer.voxelVolume[VoxelBufferId::Emissive]);
				injection.descriptors[0]->setTexture("uShadowMap", shadowMapData.shadowTexture);

				injection.descriptors[0]->update();
			}
		}
		
		namespace update_radiance
		{
			using Entity = ecs::Chain
				::With<component::UpdateRadiance>
				::Write<component::Voxelization>
				::To<ecs::Entity>;

			using Query = ecs::Chain
				::Read<component::Light>
				::To<ecs::Query>;

			inline auto system(Entity entity, 
				Query lightQuery,
				global::component::VoxelBuffer& buffer,
				global::component::VoxelRadianceInjection& injection,
				const component::RendererData& renderData, 
				ecs::World world)
			{
			
				auto [voxel] = entity;

				if (!voxel.dirty)
					return;

				buffer.voxelVolume[VoxelBufferId::Radiance]->clear();

				PipelineInfo pipelineInfo;
				pipelineInfo.shader = injection.shader;

				pipelineInfo.groupCountX = component::Voxelization::voxelDimension / injection.shader->getLocalSizeX();
				pipelineInfo.groupCountY = component::Voxelization::voxelDimension / injection.shader->getLocalSizeY();
				pipelineInfo.groupCountZ = component::Voxelization::voxelDimension / injection.shader->getLocalSizeZ();

				auto pipeline = Pipeline::get(pipelineInfo);
				pipeline->bind(renderData.commandBuffer);
				Renderer::bindDescriptorSets(pipeline.get(), renderData.commandBuffer, 0, injection.descriptors);
				Renderer::dispatch(
					renderData.commandBuffer, 
					pipelineInfo.groupCountX, 
					pipelineInfo.groupCountY, 
					pipelineInfo.groupCountZ
				);
				pipeline->end(renderData.commandBuffer);

				Renderer::memoryBarrier(renderData.commandBuffer,
					MemoryBarrierFlags::Shader_Image_Access_Barrier |
					MemoryBarrierFlags::Texture_Fetch_Barrier);

				voxel.dirty = false;

				Application::getRenderDoc().endCapture();
			}
		}

		namespace voxelize_dynamic_scene
		{
			using Entity = ecs::Chain
				::Write<component::Voxelization>
				::To<ecs::Entity>;

			inline auto system(Entity entity, const component::RendererData & renderData, ecs::World world)
			{
				
			}
		}

		namespace voxelize_static_scene 
		{
			using Entity = ecs::Chain
				::Write<component::Voxelization>
				::To<ecs::Entity>;

			inline auto system(Entity entity, 
				global::component::VoxelBuffer & buffer,
				component::CameraView & cameraView,
				component::RendererData & renderData,
				component::DeferredData& deferredData,
				capture_graph::component::RenderGraph & graph,
				ecs::World world)
			{
	
				auto [voxel] = entity;
				if (!voxel.dirty)
					return;

				for (auto text : buffer.voxelVolume) 
				{
					text->clear();
				}
				//voxelize the whole scene now. this would be optimize in the future.
				//voxelization the inner room is a good choice.
				buffer.descriptors[DescriptorID::VertexUniform]->setUniform("UniformBufferObjectVert", "projView", &cameraView.projView);
				buffer.descriptors[DescriptorID::VertexUniform]->update();

				buffer.descriptors[DescriptorID::FragmentUniform]->setTexture("uVoxelAlbedo", buffer.voxelVolume[VoxelBufferId::Albedo]);
				buffer.descriptors[DescriptorID::FragmentUniform]->setTexture("uVoxelNormal", buffer.voxelVolume[VoxelBufferId::Normal]);
				buffer.descriptors[DescriptorID::FragmentUniform]->setTexture("uVoxelEmission", buffer.voxelVolume[VoxelBufferId::Emissive]);
				buffer.descriptors[DescriptorID::FragmentUniform]->setTexture("uStaticVoxelFlag", buffer.staticFlag);
				buffer.descriptors[DescriptorID::FragmentUniform]->update();


				PipelineInfo pipeInfo;
				pipeInfo.shader = buffer.voxelShader;
				pipeInfo.cullMode = CullMode::None;
				pipeInfo.depthTest = false;
				pipeInfo.clearTargets = true;
				pipeInfo.colorTargets[0] = buffer.colorBuffer;
				
				auto pipeline = Pipeline::get(pipeInfo,buffer.descriptors, graph);
				pipeline->bind(renderData.commandBuffer);
				
				for (auto & cmd : deferredData.commandQueue)
				{
					cmd.material->setShader(buffer.voxelShader);
					cmd.material->bind();
					buffer.descriptors[DescriptorID::MaterialBinding] = cmd.material->getDescriptorSet();

					Renderer::bindDescriptorSets(pipeline.get(), renderData.commandBuffer, 0, buffer.descriptors);
					Renderer::drawMesh(renderData.commandBuffer, pipeline.get(), cmd.mesh);
				}

				pipeline->end(renderData.commandBuffer);

				Renderer::memoryBarrier(renderData.commandBuffer, 
					MemoryBarrierFlags::Shader_Image_Access_Barrier | 
					MemoryBarrierFlags::Texture_Fetch_Barrier);

			
			}//next is dynamic
		}

		namespace setup_voxel_volumes
		{
			using Entity = ecs::Chain
				::Write<component::Voxelization>
				::To<ecs::Entity>;

			inline auto system(Entity entity, 
				global::component::VoxelBuffer& voxelBuffer,
				global::component::VoxelRadianceInjection& injection,
				component::BoundingBoxComponent & box,
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
						voxelBuffer.volumeGridSize = glm::max(axisSize.x, glm::max(axisSize.y, axisSize.z));
						voxelBuffer.voxelSize = voxelBuffer.volumeGridSize / component::Voxelization::voxelDimension;

						injection.uniformData.volumeDimension = component::Voxelization::voxelDimension;
						injection.uniformData.worldMinPoint = { voxelBuffer.box.min ,1.f };
						injection.uniformData.voxelSize = voxelBuffer.voxelSize;
						injection.uniformData.voxelScale = 1 / voxelBuffer.volumeGridSize;

						auto halfSize = voxelBuffer.volumeGridSize / 2.0f;
						// projection matrix
						auto projection = glm::ortho(-halfSize, halfSize, -halfSize, halfSize, 0.0f, voxelBuffer.volumeGridSize);
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

						const float voxelScale = 1.f / voxelBuffer.volumeGridSize;
						const float dimension = component::Voxelization::voxelDimension;

						voxelBuffer.descriptors[DescriptorID::GeometryUniform]->setUniform("UniformBufferGemo", "viewProjections", &voxelBuffer.viewProj);
						voxelBuffer.descriptors[DescriptorID::GeometryUniform]->setUniform("UniformBufferGemo", "viewProjectionsI", &voxelBuffer.viewProjInverse);
						voxelBuffer.descriptors[DescriptorID::GeometryUniform]->setUniform("UniformBufferGemo", "worldMinPoint", &box.box->min);
						voxelBuffer.descriptors[DescriptorID::GeometryUniform]->setUniform("UniformBufferGemo", "voxelScale", &voxelScale);
						voxelBuffer.descriptors[DescriptorID::GeometryUniform]->setUniform("UniformBufferGemo", "volumeDimension", &dimension);
						voxelBuffer.descriptors[DescriptorID::GeometryUniform]->update();
						
						uint32_t flagVoxel = 1;
						voxelBuffer.descriptors[DescriptorID::FragmentUniform]->setUniform("UniformBufferObject", "flagStaticVoxels", &flagVoxel);
						voxelBuffer.descriptors[DescriptorID::FragmentUniform]->update();

						voxelization.dirty = true;
					}
				}
			}
		}

		auto registerGlobalComponent(std::shared_ptr<ExecutePoint> point) -> void
		{
			point->registerGlobalComponent<global::component::VoxelBuffer>(&initVoxelBuffer);
			point->registerGlobalComponent<global::component::VoxelRadianceInjection>([](global::component::VoxelRadianceInjection & injection) {
				injection.shader = Shader::create("shaders/VXGI/InjectRadiance.shader");
				injection.descriptors.emplace_back(
					DescriptorSet::create({ 0, injection.shader.get() })
				);
			});
		}

		auto registerVoxelizer(ExecuteQueue& begin, ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> point) -> void
		{
			point->registerWithinQueue<begin_scene::system>(begin);
			point->registerWithinQueue<setup_voxel_volumes::system>(renderer);
			point->registerWithinQueue<voxelize_static_scene::system>(renderer);
			point->registerWithinQueue<voxelize_dynamic_scene::system>(renderer);
			point->registerWithinQueue<update_radiance::system>(renderer);
		}
	}
};