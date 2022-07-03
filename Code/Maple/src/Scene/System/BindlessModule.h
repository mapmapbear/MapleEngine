//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <memory>
#include "Scene/System/ExecutePoint.h"

namespace maple
{
	namespace bindless
	{
		auto registerBindless(std::shared_ptr<ExecutePoint> executePoint) -> void;
	}
}        // namespace maple