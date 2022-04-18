//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "Voxelization.h"
#include "Engine/Renderer/RendererData.h"
#include "Scene/System/ExecutePoint.h"
#include "Scene/Component/BoundingBox.h"

#include <array>

namespace maple
{
	namespace component 
	{
		struct VoxelBuffer
		{
			std::array<glm::mat4, 3> viewProj;
			std::array<glm::mat4, 3> viewProjInverse;
			BoundingBox box;
		};
	}

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
					voxelization.volumeGridSize = glm::max(axisSize.x, glm::max(axisSize.y, axisSize.z));
					voxelization.voxelSize = voxelization.volumeGridSize / voxelization.volumeDimension;
					auto halfSize = voxelization.volumeGridSize / 2.0f;
					// projection matrix
					auto projection = glm::ortho(-halfSize, halfSize, -halfSize, halfSize, 0.0f,
						voxelization.volumeGridSize);
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
						buffer.viewProjInverse[i++] = inverse(matrix);
					}
				}
			}
		}
	}

	auto registerVoxelizer(ExecuteQueue& begin, ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> point) -> void
	{
		point->registerGlobalComponent<component::Voxelization>();
		point->registerGlobalComponent<component::VoxelBuffer>();
		point->registerWithinQueue<setup_voxel_volumes::system>(begin);
	}
};