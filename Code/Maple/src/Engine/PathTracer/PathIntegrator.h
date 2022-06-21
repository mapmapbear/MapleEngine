//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Scene/System/ExecutePoint.h"
#include <cstdint>

namespace maple 
{
	namespace component 
	{
		struct PathIntegrator 
		{
			int32_t depth = 8;
		};
	}

	namespace path_integrator 
	{
		auto registerPathIntegrator(ExecuteQueue& begin, ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> executePoint) -> void;
	}
}