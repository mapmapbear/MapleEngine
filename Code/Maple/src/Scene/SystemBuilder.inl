//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Animation/AnimationSystem.h"

namespace maple
{
	inline auto registerSystem(std::shared_ptr<ExecutePoint> executePoint)
	{
		animation::registerAnimationSystem(executePoint);
	}
}
