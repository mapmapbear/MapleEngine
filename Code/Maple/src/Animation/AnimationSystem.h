//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Scene/System/ExecutePoint.h"

namespace maple
{
	namespace animation 
	{
		auto registerAnimationSystem(std::shared_ptr<ExecutePoint> executePoint) -> void;
	}
};