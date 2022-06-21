//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "DDGIRenderer.h"
#include "Engine/Raytrace/RaytraceScale.h"
#include "IrradianceVolume.h"
#include "RHI/Texture.h"

namespace maple
{
	namespace ddgi
	{
		namespace component
		{
			struct DDGIPipeline
			{
				float         width;
				float         height;
				RaytraceScale scale;
			};

			struct ProbeGrid
			{
				float      probeDistance               = 1.0f;
				float      recursiveEnergyPreservation = 0.85f;
				uint32_t   irradianceOctSize           = 8;
				uint32_t   depthOctSize                = 16;
				glm::vec3  gridStartPosition;
				glm::ivec3 probeCounts;

				std::vector<std::shared_ptr<DescriptorSet>> writeSets;
				std::vector<std::shared_ptr<DescriptorSet>> readSets;
			};

		}        // namespace component

		namespace on_init
		{
			using Entity = ecs::Registry ::Include<component::IrradianceVolume>::Exclude<component::DDGIPipeline>::To<ecs::Entity>;

			inline auto system(Entity entity, ecs::World world)
			{
				auto &pipeline = world.addComponent<component::DDGIPipeline>(entity);
			}
		}        // namespace on_init

		namespace begin_scene
		{
			using Entity = ecs::Registry ::Modify<component::IrradianceVolume>::To<ecs::Entity>;

			inline auto system(Entity entity)
			{
				auto [irradiance] = entity;
			}
		}        // namespace begin_scene
	}            // namespace ddgi

	namespace ddgi
	{
		auto registerDDGI(ExecuteQueue &begin, std::shared_ptr<ExecutePoint> point) -> void
		{
		}
	}        // namespace ddgi
}        // namespace maple