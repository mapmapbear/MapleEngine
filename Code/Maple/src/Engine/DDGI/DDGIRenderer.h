//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Engine/Core.h"
#include "Scene/System/ExecutePoint.h"
#include "Engine/Raytrace/RaytraceScale.h"
#include <glm/glm.hpp>

namespace maple
{
	namespace ddgi
	{
		constexpr uint32_t IrradianceOctSize = 8;
		constexpr uint32_t DepthOctSize      = 16;

		namespace component
		{
			struct DDGIUniform
			{
				glm::vec4  startPosition;
				glm::vec4  step;//align
				glm::ivec4 probeCounts;

				float maxDistance    = 6.f;
				float depthSharpness = 50.f;
				float hysteresis     = 0.98f;
				float normalBias     = 1.0f;

				float   energyPreservation        = 0.85f;
				int32_t irradianceProbeSideLength = IrradianceOctSize;
				int32_t irradianceTextureWidth;
				int32_t irradianceTextureHeight;

				int32_t depthProbeSideLength = DepthOctSize;
				int32_t depthTextureWidth;
				int32_t depthTextureHeight;
				int32_t raysPerProbe = 256;
			};

			struct DDGIPipeline
			{
				float   probeDistance           = 4.f;
				bool    infiniteBounce          = true;
				int32_t raysPerProbe            = 256;
				float   hysteresis              = 0.98f;
				float   intensity               = 1.f;
				float   infiniteBounceIntensity = 1.7f;
				float   normalBias              = 1.f;
				float   depthSharpness          = 50.f;
				float   energyPreservation      = 0.85f;

				float             width;
				float             height;
				RaytraceScale::Id scale = RaytraceScale::Full;
			};

			struct ApplyEvent
			{
				uint8_t dummy;
			};
		}        // namespace component

		auto registerDDGI(ExecuteQueue &begin, ExecuteQueue &render, std::shared_ptr<ExecutePoint> point) -> void;
	}        // namespace ddgi
}        // namespace maple