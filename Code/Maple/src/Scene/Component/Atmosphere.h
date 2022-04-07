//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Component.h"
#include <glm/glm.hpp>
namespace maple
{
	namespace component 
	{
		class Atmosphere : public Component
		{
		public:
			constexpr static float     SURFACE_RADIUS = 6360;
			constexpr static float     ATMOSPHE_RERADIUS = 6420;
			constexpr static float     G = 0.76;
			constexpr static glm::vec3 RAYLEIGH_SCATTERING = glm::vec3(58e-7f, 135e-7f, 331e-7f);
			constexpr static glm::vec3 MIE_SCATTERING = glm::vec3(2.2e-5f);

			Atmosphere() = default;

			struct AtmosphereData
			{
				float surfaceRadius = SURFACE_RADIUS;
				float atmosphereRadius = ATMOSPHE_RERADIUS;
				float g = G;

				glm::vec3 rayleighScattering = RAYLEIGH_SCATTERING;
				glm::vec3 mieScattering = MIE_SCATTERING;
				glm::vec3 centerPoint = glm::vec3(0.f, -SURFACE_RADIUS, 0.f);
			};

			inline auto& getData() const
			{
				return data;
			}

			inline auto& getData()
			{
				return data;
			}

			bool renderToScreen = true;

		private:
			AtmosphereData data;
		};
	}
};        // namespace maple
