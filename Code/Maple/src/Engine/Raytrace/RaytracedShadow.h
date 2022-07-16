//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "RHI/Texture.h"
#include "RaytraceScale.h"
#include "Scene/System/ExecutePoint.h"

namespace maple
{
	namespace raytraced_shadow
	{
		namespace component
		{
			struct RaytracedShadow
			{
				RaytraceScale::Id scale = RaytraceScale::Full;
				uint32_t          width;
				uint32_t          height;
			};
		}        // namespace component
		auto registerRaytracedShadow(ExecuteQueue &update, ExecuteQueue &queue, std::shared_ptr<ExecutePoint> executePoint) -> void;
	};        // namespace raytraced_shadow
};            // namespace maple
