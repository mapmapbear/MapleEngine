//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "DDGIRenderer.h"
#include "IrradianceVolume.h"
#include "RHI/Texture.h"
#include "Engine/Raytrace/RaytraceScale.h"

namespace maple
{
	namespace ddgi
	{
		namespace component 
		{
			struct DDGIPipeline 
			{
				float width;
				float height;
				RaytraceScale scale;
			};

			struct ProbeGrid
			{
				float       probeDistance = 1.0f;
				float       recursiveEnergyPreservation = 0.85f;
				uint32_t    irradianceOctSize = 8;
				uint32_t    depthOctSize = 16;
				glm::vec3   gridStartPosition;
				glm::ivec3  probeCounts;

				std::vector<std::shared_ptr<DescriptorSet>> writeSets;
				std::vector<std::shared_ptr<DescriptorSet>> readSets;
			};

		}

		namespace on_init
		{
			using Entity = ecs::Chain
				::With<component::IrradianceVolume>
				::Without<component::DDGIPipeline>
				::To<ecs::Entity>;

			inline auto system(Entity entity, ecs::World world)
			{
				auto& pipeline = world.addComponent<component::DDGIPipeline>(entity);

			}
		}


		namespace begin_scene
		{
			using Entity = ecs::Chain
				::Write<component::IrradianceVolume>
				::To<ecs::Entity>;

			inline auto system(Entity entity)
			{
				auto [irradiance] = entity;
			}
		}
	}


	namespace ddgi
	{
		auto registerDDGI(ExecuteQueue& begin, std::shared_ptr<ExecutePoint> point) -> void
		{
			
		}
	}
}