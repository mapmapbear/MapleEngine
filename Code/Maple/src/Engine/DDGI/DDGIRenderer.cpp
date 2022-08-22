//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "DDGIRenderer.h"
#include "Engine/Raytrace/RaytraceScale.h"
#include "IrradianceVolume.h"
#include "RHI/Texture.h"
#include "Scene/Component/BoundingBox.h"

namespace maple
{
	namespace ddgi
	{
		namespace component
		{
			struct DDGIPipelineInternal
			{
				BoundingBox box;

				std::vector<std::shared_ptr<DescriptorSet>> writeSets;
				std::vector<std::shared_ptr<DescriptorSet>> readSets;
			};
		}        // namespace component

		namespace init
		{
			inline auto updateUniform(ddgi::component::DDGIPipeline &pipeline, component::DDGIUniform &uniform, BoundingBox *box)
			{
				if (box != nullptr)
				{
					glm::vec3 sceneLength = box->max - box->min;
					// Add 2 more probes to fully cover scene.
					uniform.probeCounts        = glm::ivec3(sceneLength / pipeline.probeDistance) + glm::ivec3(2);
					uniform.startPosition      = box->min;
					uniform.maxDistance        = pipeline.probeDistance * 1.5f;
					uniform.depthSharpness     = pipeline.depthSharpness;
					uniform.hysteresis         = pipeline.hysteresis;
					uniform.normalBias         = pipeline.normalBias;
					uniform.energyPreservation = pipeline.energyPreservation;
					//todo update uniform
				}
			}

			inline auto initDDGIPipeline(ddgi::component::DDGIPipeline &pipeline, Entity entity, ecs::World world)
			{
				auto &bBox = world.getComponent<maple::component::BoundingBoxComponent>();
				if (bBox.box != nullptr)
				{
					entity.addComponent<ddgi::component::DDGIPipelineInternal>();
				}
			}
		}        // namespace init

		namespace on_update
		{
			inline auto onUniformChanged(ddgi::component::DDGIPipeline &pipeline, Entity entity, ecs::World world)
			{
				auto &uniform = world.getComponent<component::DDGIUniform>(entity);
				auto &bBox    = world.getComponent<maple::component::BoundingBoxComponent>();
				init::updateUniform(pipeline, uniform, bBox.box);
			}
		}        // namespace on_update
	}            // namespace ddgi

	namespace ddgi
	{
		// clang-format off
		using Entity = ecs::Registry 
			::Modify<ddgi::component::DDGIPipeline>
			::Exclude<ddgi::component::DDGIPipelineInternal>
			::To<ecs::Entity>;
		// clang-format on

		inline auto system(Entity entity, const maple::component::BoundingBoxComponent &box, ecs::World world)
		{
			auto [pipeline] = entity;

			if (box.box != nullptr)
			{
				world.addComponent<ddgi::component::DDGIPipelineInternal>(entity);
			}
		}
	}        // namespace ddgi

	namespace ddgi
	{
		auto registerDDGI(ExecuteQueue &begin, ExecuteQueue &render, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->onConstruct<ddgi::component::DDGIPipeline, init::initDDGIPipeline>();
			executePoint->onUpdate<ddgi::component::DDGIPipeline, on_update::onUniformChanged>();
		}
	}        // namespace ddgi
}        // namespace maple