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
		struct Atmosphere
		{
			constexpr static float     SURFACE_RADIUS = 6360.f;
			constexpr static float     ATMOSPHE_RERADIUS = 6420.f;
			constexpr static float     G = 0.76f;
			constexpr static glm::vec3 RAYLEIGH_SCATTERING = glm::vec3(58e-7f, 135e-7f, 331e-7f);
			constexpr static glm::vec3 MIE_SCATTERING = glm::vec3(2.2e-5f);

			float surfaceRadius = SURFACE_RADIUS;
			float atmosphereRadius = ATMOSPHE_RERADIUS;
			float g = G;
			glm::vec3 rayleighScattering = RAYLEIGH_SCATTERING;
			glm::vec3 mieScattering = MIE_SCATTERING;
			glm::vec3 centerPoint = glm::vec3(0.f, -SURFACE_RADIUS, 0.f);
			bool renderToScreen = true;
		};
	}
};        // namespace maple
