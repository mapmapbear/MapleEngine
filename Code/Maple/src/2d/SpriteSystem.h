//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Scene/System/ExecutePoint.h"

namespace maple 
{
	namespace sprite2d 
	{
		auto registerSprite2dModule(std::shared_ptr<ExecutePoint> executePoint) -> void;
	}
};