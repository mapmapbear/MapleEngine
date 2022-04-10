//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include <memory>

namespace maple
{
	class ExecutePoint;

	namespace physics 
	{
		auto registerPhysicsModule(std::shared_ptr<ExecutePoint> executePoint) -> void;
	}
}