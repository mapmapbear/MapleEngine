//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "AnimationSystem.h"

namespace maple
{
	namespace animation 
	{
		inline auto system(ecs::World world) 
		{
			
		}

		auto registerAnimationSystem(std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerSystem<animation::system>();
		}
	}
};
