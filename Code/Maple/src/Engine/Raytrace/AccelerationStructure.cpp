//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "AccelerationStructure.h"
#include "Engine/Mesh.h"
#include "Engine/Renderer/RendererData.h"
#include "RHI/AccelerationStructure.h"
#include "Scene/Component/Bindless.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Component/Transform.h"


namespace maple
{
	namespace
	{
		constexpr uint32_t MAX_SCENE_MESH_INSTANCE_COUNT = 1024;
	}

	namespace raytracing
	{
		namespace update
		{
			using Entity = ecs::Registry::Modify<global::component::TopLevelAs>::To<ecs::Entity>;

			using MeshQuery = ecs::Registry ::Modify<component::MeshRenderer>::Modify<component::Transform>::To<ecs::Group>;

			inline auto system(Entity                                           entity,
			                   MeshQuery                                        meshGroup,
			                   const maple::component::RendererData &           renderData,
			                   maple::global::component::SceneTransformChanged *sceneChanged,
			                   maple::global::component::Bindless &bindless, ecs::World world)
			{
				auto [topLevel] = entity;

				if (topLevel.topLevelAs == nullptr)
				{
					if (!meshGroup.empty())
					{
						topLevel.topLevelAs = AccelerationStructure::createTopLevel(MAX_SCENE_MESH_INSTANCE_COUNT);
						auto     tasks      = BatchTask::create();
						uint32_t meshCount  = 0;

						for (auto meshEntity : meshGroup)
						{
							auto [mesh, transform]                   = meshGroup.convert(meshEntity);
							auto blas                                = mesh.mesh->getAccelerationStructure(tasks);
							bindless.meshIndices[mesh.mesh->getId()] = meshCount;
							topLevel.topLevelAs->updateTLAS(transform.getWorldMatrix(), meshCount++, blas->getDeviceAddress());
						}
						topLevel.topLevelAs->copyToGPU(renderData.commandBuffer, meshCount, 0);
						topLevel.topLevelAs->build(renderData.commandBuffer, meshCount);
						tasks->execute();
					}
				}
				else if (sceneChanged && sceneChanged->dirty)
				{
					auto     tasks     = BatchTask::create();
					uint32_t meshCount = 0;
					//todo...
					for (auto meshEntity : meshGroup)
					{
						auto [mesh, transform]                   = meshGroup.convert(meshEntity);
						auto blas                                = mesh.mesh->getAccelerationStructure(tasks);
						bindless.meshIndices[mesh.mesh->getId()] = meshCount;
						topLevel.topLevelAs->updateTLAS(transform.getWorldMatrix(), meshCount++, blas->getDeviceAddress());
					}
					if (meshCount > 0)
					{
						topLevel.topLevelAs->copyToGPU(renderData.commandBuffer, meshCount, 0);
						topLevel.topLevelAs->build(renderData.commandBuffer, meshCount);
						tasks->execute();
					}
				


					/*auto tasks = BatchTask::create();
					bool needUpdate = false;
					for (auto entity : sceneChanged->entities)
					{
						auto meshRender = world.tryGetComponent<component::MeshRenderer>(entity);
						auto transform  = world.tryGetComponent<component::Transform>(entity);
						if (meshRender && transform)
						{
							auto blas = meshRender->mesh->getAccelerationStructure(tasks);

							if (auto iter = bindless.meshIndices.find(meshRender->mesh->getId()); iter == bindless.meshIndices.end())
							{
								bindless.meshIndices[meshRender->mesh->getId()] = bindless.meshIndices.size();
							}

							auto offset = topLevel.topLevelAs->updateTLAS(
								transform->getWorldMatrix(), 
								bindless.meshIndices[meshRender->mesh->getId()], 
								blas->getDeviceAddress()
							);
							topLevel.topLevelAs->copyToGPU(renderData.commandBuffer, 1, offset);
							needUpdate = true;
						}
					}
					if (needUpdate)
						topLevel.topLevelAs->build(renderData.commandBuffer, bindless.meshIndices.size());
					tasks->execute();*/
				}
			}
		}        // namespace update

		auto registerAccelerationStructureModule(ExecuteQueue &begin, std::shared_ptr<ExecutePoint> point) -> void
		{
			point->registerGlobalComponent<global::component::TopLevelAs>();
			point->registerWithinQueue<update::system>(begin);
		}
	}        // namespace raytracing
}        // namespace maple