//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Animation/AnimationSystem.h"
#include "2d/SpriteSystem.h"
#include "Scripts/Mono/MonoSystem.h"

namespace maple
{
	inline auto registerSystem(std::shared_ptr<ExecutePoint> executePoint)
	{
		animation::registerAnimationModule(executePoint);
		sprite2d::registerSprite2dModule(executePoint);
		mono::registerMonoModule(executePoint);
	}
}
