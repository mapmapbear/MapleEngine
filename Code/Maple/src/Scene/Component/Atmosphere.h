//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Component.h"
#include <glm/glm.hpp>
namespace maple
{
	class Atmosphere : public Component
	{
	  public:
		constexpr static char *ICON = ICON_MDI_WEATHER_SUNNY;

		Atmosphere() = default;
		struct AtmosphereData
		{
			glm::vec3 viewPos;                 //vec3 pos
			int       viewSamples = 16;        //view samples

			glm::vec3 sunDir       = glm::vec3(0, 1, 0);        //vec3 pos
			int       lightSamples = 8;                         //light samples

			float inensitySun = 20.f;         // Intensity of the sun
			float R_e         = 6360.;        // Radius of the planet [m]
			float R_a         = 6420.;        // Radius of the atmosphere [m]
			float padding1    = 0;

			glm::vec3 beta_R = {5.8e-3f, 13.5e-3f, 33.1e-3f};        // Rayleigh scattering coefficient
			float     beta_M = 21e-3f;                               // Mie scattering coefficient

			float H_R      = 7.994f;        // Rayleigh scale height  7994, 100
			float H_M      = 1.200f;        // Mie scale height  1200, 20
			float g        = 0.888f;        // Mie scattering direction -  anisotropy of the medium
			float padding2 = 0;
		};
		inline auto &getData() const
		{
			return data;
		}

		inline auto &getData()
		{
			return data;
		}

		inline auto &getAngle() const
		{
			return angle;
		}

		inline auto setAngle(float angle)
		{
			this->angle   = angle;
			data.sunDir.y = glm::sin(angle);
			data.sunDir.z = -glm::cos(angle);
		}

	  private:
		AtmosphereData data;
		float          angle = 0;
	};
};        // namespace maple
