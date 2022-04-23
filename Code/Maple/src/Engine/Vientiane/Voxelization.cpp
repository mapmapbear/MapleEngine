//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "Voxelization.h"
#include "Engine/Renderer/RendererData.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/DeferredOffScreenRenderer.h"

#include "Scene/System/ExecutePoint.h"
#include "Scene/Component/BoundingBox.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Component/Transform.h"

#include "RHI/Texture.h"
#include "RHI/Shader.h"
#include "RHI/Pipeline.h"

#include <array>

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
		Length
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
					{ TextureFormat::RGBA8, TextureFilter::Linear,TextureWrap::ClampToEdge },
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

			buffer.voxelShader = Shader::create("shaders/VXGI/Voxelization.shader");
			buffer.descriptors.resize(3);
			for (uint32_t i = 0;i<3;i++)
			{
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

			inline auto system(Entity entity, Query meshQuery, global::component::VoxelBuffer & buffer, ecs::World world)
			{
				auto [voxel] = entity;

				if (!voxel.dirty)
					return;

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

		namespace update_radiance
		{
			using Entity = ecs::Chain
				::With<component::UpdateRadiance>
				::To<ecs::Entity>;

			inline auto system(Entity entity, const component::RendererData& renderData, ecs::World world)
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
				buffer.descriptors[DescriptorID::VertexUniform]->setUniform("UniformBufferObject", "projView", &cameraView.projView);
				buffer.descriptors[DescriptorID::VertexUniform]->update();

				PipelineInfo pipeInfo;
				pipeInfo.shader = buffer.voxelShader;
				pipeInfo.cullMode = CullMode::None;
				pipeInfo.depthTest = false;
				
				auto pipeline = Pipeline::get(pipeInfo);
				pipeline->bind(renderData.commandBuffer);
				
				for (auto & cmd : deferredData.commandQueue)
				{
					buffer.descriptors[DescriptorID::MaterialBinding] = cmd.material->getDescriptorSet();
					cmd.material->setShader(buffer.voxelShader);
					cmd.material->bind();

					Renderer::bindDescriptorSets(pipeline.get(), renderData.commandBuffer, 0, buffer.descriptors);
					Renderer::drawMesh(renderData.commandBuffer, pipeline.get(), cmd.mesh);
				}

				pipeline->end(renderData.commandBuffer);
				Renderer::memoryBarrier(renderData.commandBuffer, 
					MemoryBarrierFlags::Shader_Image_Access_Barrier | 
					MemoryBarrierFlags::Texture_Fetch_Barrier);

				voxel.dirty = false;
			}//next is dynamic
		}

		namespace setup_voxel_volumes
		{
			using Entity = ecs::Chain
				::Write<component::Voxelization>
				::To<ecs::Entity>;

			inline auto system(Entity entity, 
				global::component::VoxelBuffer& voxelBuffer,
				component::BoundingBoxComponent & box,
				ecs::World world)
			{
				auto [voxelization] = entity;
				if (box.box) 
				{
					if (voxelBuffer.box != *box.box)
					{
						voxelBuffer.box = *box.box;

						auto axisSize = voxelBuffer.box.size();
						auto center = voxelBuffer.box.center();
						voxelBuffer.volumeGridSize = glm::max(axisSize.x, glm::max(axisSize.y, axisSize.z));
						voxelBuffer.voxelSize = voxelBuffer.volumeGridSize / component::Voxelization::voxelDimension;
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

						voxelBuffer.descriptors[DescriptorID::GeometryUniform]->setUniform("UniformBufferGemo", "viewProjections", &voxelBuffer.viewProj);
						voxelBuffer.descriptors[DescriptorID::GeometryUniform]->setUniform("UniformBufferGemo", "viewProjectionsI", &voxelBuffer.viewProjInverse);
						voxelBuffer.descriptors[DescriptorID::GeometryUniform]->setUniform("UniformBufferGemo", "worldMinPoint", &box.box->min);
						voxelBuffer.descriptors[DescriptorID::GeometryUniform]->setUniform("UniformBufferGemo", "voxelScale", &voxelScale);
						voxelBuffer.descriptors[DescriptorID::GeometryUniform]->setUniform("UniformBufferGemo", "volumeDimension", &component::Voxelization::voxelDimension);
						voxelBuffer.descriptors[DescriptorID::GeometryUniform]->update();
						
						uint32_t flagVoxel = 1;
						voxelBuffer.descriptors[DescriptorID::FragmentUniform]->setUniform("UniformBufferObject", "volumeDimension", &component::Voxelization::voxelDimension);
						voxelBuffer.descriptors[DescriptorID::FragmentUniform]->setUniform("UniformBufferObject", "flagStaticVoxels", &flagVoxel);
						voxelBuffer.descriptors[DescriptorID::FragmentUniform]->update();

						voxel.dirty = = true;
					}
				}
			}
		}

		auto registerGlobalComponent(std::shared_ptr<ExecutePoint> point) -> void
		{
			point->registerGlobalComponent<component::Voxelization>();
			point->registerGlobalComponent<global::component::VoxelBuffer>(&initVoxelBuffer);
		}

		auto registerVoxelizer(ExecuteQueue& begin, ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> point) -> void
		{
			point->registerWithinQueue<setup_voxel_volumes::system>(begin);
			point->registerWithinQueue<voxelize_static_scene::system>(begin);
			point->registerWithinQueue<voxelize_dynamic_scene::system>(begin);
		}
	}
};