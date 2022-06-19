//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Scene/System/ExecutePoint.h"
#include "RaytraceScale.h"
#include "RHI/Texture.h"

namespace maple
{
	namespace raytraced_shadow
	{
		namespace component 
		{
			struct RaytracedShadow
			{
				RaytraceScale scale = RaytraceScale::Half;
				uint32_t width;
				uint32_t height;
			};
		}
		auto registerRaytracedShadow(ExecuteQueue& update, ExecuteQueue& queue, std::shared_ptr<ExecutePoint> executePoint) -> void;
	};
};        // namespace maple
