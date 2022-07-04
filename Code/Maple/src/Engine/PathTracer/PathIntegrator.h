//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Scene/System/ExecutePoint.h"
#include <cstdint>

namespace maple
{
	class Texture2D;

	namespace component
	{
		struct PathIntegrator
		{
			int32_t                    readIndex = 0;
			int32_t                    depth     = 8;
			int32_t                    maxBounces = 8;
			uint32_t                   accumulatedSamples = 0;
			float                      shadowRayBias      = 0.0000;
			std::shared_ptr<Texture2D> images[2];
		};
	}        // namespace component

	namespace path_integrator
	{
		auto registerPathIntegrator(ExecuteQueue &begin, ExecuteQueue &renderer, std::shared_ptr<ExecutePoint> executePoint) -> void;
	}
}        // namespace maple