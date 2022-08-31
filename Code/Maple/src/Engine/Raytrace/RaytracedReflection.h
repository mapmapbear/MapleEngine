//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "RHI/Texture.h"
#include "RaytraceScale.h"
#include "Scene/System/ExecutePoint.h"

namespace maple
{
	namespace raytraced_reflection
	{
		namespace component
		{
			struct RaytracedReflection
			{
				RaytraceScale::Id scale = RaytraceScale::Full;
				uint32_t          width;
				uint32_t          height;
				Texture2D::Ptr    output;
				bool              enable = true;
			};
		}        // namespace component
		auto registerRaytracedReflection(ExecuteQueue &update, ExecuteQueue &queue, std::shared_ptr<ExecutePoint> executePoint) -> void;
	};        // namespace raytraced_reflection
};            // namespace maple
