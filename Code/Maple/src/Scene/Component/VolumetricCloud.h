//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Component.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace maple
{
	namespace component
	{
		class VolumetricCloud : public Component
		{
		public:

			float cloudSpeed = 450.0;
			float coverage = 0.45;
			float crispiness = 40.;
			float curliness = .1;
			float density = 0.02;
			float absorption = 0.35;

			float earthRadius = 600000.0;
			float sphereInnerRadius = 5000.0;
			float sphereOuterRadius = 17000.0;

			float perlinFrequency = 0.8;

			bool enableGodRays;
			bool enablePowder;
			bool postProcess;

			template <class Archive>
			inline auto serialize(Archive& archive) -> void
			{
				archive(
					cloudSpeed,
					coverage,
					crispiness,
					curliness,
					density,
					absorption,
					earthRadius,
					sphereInnerRadius,
					sphereOuterRadius,
					perlinFrequency,
					enableGodRays,
					enablePowder,
					postProcess,
					entity);
			}

			bool weathDirty = true;
		};
	};
};        // namespace maple
