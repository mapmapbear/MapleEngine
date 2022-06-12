//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "DDGIRenderer.h"
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
				std::shared_ptr<Texture2DArray> irrandiances[2];
				std::shared_ptr<Texture2DArray> moments[2];
			};
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