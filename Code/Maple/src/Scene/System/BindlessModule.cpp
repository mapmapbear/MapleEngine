//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "BindlessModule.h"
#include "Scene/Component/Bindless.h"

namespace maple
{
	namespace bindless
	{
		namespace clear_queue
		{
			using Entity = ecs::Registry::Modify<global::component::MaterialChanged>::To<ecs::Entity>;
			inline auto system(Entity entity)
			{
				auto [changed] = entity;
				if (!changed.updateQueue.empty())
				{
					changed.updateQueue.clear();
				}
			}
		}

		auto registerBindless(std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerSystemInFrameEnd<clear_queue::system>();
		}
	}
}        // namespace maple