//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "Voxelization.h"
#include "Engine/Renderer/RendererData.h"
#include "Scene/System/ExecutePoint.h"
#include "Scene/Component/BoundingBox.h"
#include "RHI/Texture.h"

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
		};
	}

	namespace 
	{
		inline auto initVoxelBuffer(component::VoxelBuffer & buffer)
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
		}
	}

	namespace vxgi
	{
		namespace voxelize_dynamic_scene
		{
			using Entity = ecs::Chain
				::Write<component::Voxelization>
				::Read<component::RendererData>
				::To<ecs::Entity>;

			inline auto system(Entity entity, ecs::World world)
			{

			}
		}

		namespace voxelize_static_scene 
		{
			using Entity = ecs::Chain
				::Write<component::Voxelization>
				::Read<component::RendererData>
				::To<ecs::Entity>;

			inline auto system(Entity entity, ecs::World world)
			{

			}
		}

		namespace setup_voxel_volumes
		{
			using Entity = ecs::Chain
				::Write<component::Voxelization>
				::Write<component::VoxelBuffer>
				::Read<component::BoundingBoxComponent>
				::To<ecs::Entity>;

			inline auto system(Entity entity, ecs::World world)
			{
				auto [voxelization, buffer, box] = entity;
				if (box.box) 
				{
					if (buffer.box != *box.box)
					{
						buffer.box = *box.box;

						auto axisSize = buffer.box.size();
						auto center = buffer.box.center();
						buffer.volumeGridSize = glm::max(axisSize.x, glm::max(axisSize.y, axisSize.z));
						buffer.voxelSize = buffer.volumeGridSize / component::Voxelization::voxelDimension;
						auto halfSize = buffer.volumeGridSize / 2.0f;
						// projection matrix
						auto projection = glm::ortho(-halfSize, halfSize, -halfSize, halfSize, 0.0f, buffer.volumeGridSize);
						// view matrices
						buffer.viewProj[0] = lookAt(center + glm::vec3(halfSize, 0.0f, 0.0f),
							center, glm::vec3(0.0f, 1.0f, 0.0f));
						buffer.viewProj[1] = lookAt(center + glm::vec3(0.0f, halfSize, 0.0f),
							center, glm::vec3(0.0f, 0.0f, -1.0f));
						buffer.viewProj[2] = lookAt(center + glm::vec3(0.0f, 0.0f, halfSize),
							center, glm::vec3(0.0f, 1.0f, 0.0f));

						int32_t i = 0;

						for (auto& matrix : buffer.viewProj)
						{
							matrix = projection * matrix;
							buffer.viewProjInverse[i++] = glm::inverse(matrix);
						}
					}
				}
			}
		}

		auto registerGlobalComponent(std::shared_ptr<ExecutePoint> point) -> void
		{
			point->registerGlobalComponent<component::Voxelization>();
			point->registerGlobalComponent<component::VoxelBuffer>(&initVoxelBuffer);
		}

		auto registerVoxelizer(ExecuteQueue& begin, ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> point) -> void
		{
			point->registerWithinQueue<setup_voxel_volumes::system>(begin);
		}
	}
};