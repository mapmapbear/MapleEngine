//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <cstdint>
#include "RHI/Texture.h"
#include "RHI/DescriptorSet.h"

namespace maple
{
	class ExecutePoint;

	namespace blue_noise
	{
		static constexpr const char *SOBOL_TEXTURE = "textures/blue_noise/sobol_256_4d.png";

		static constexpr const char *SCRAMBLING_RANKING_TEXTURES[] = {
		    "textures/blue_noise/scrambling_ranking_128x128_2d_1spp.png",
		    "textures/blue_noise/scrambling_ranking_128x128_2d_2spp.png",
		    "textures/blue_noise/scrambling_ranking_128x128_2d_4spp.png",
		    "textures/blue_noise/scrambling_ranking_128x128_2d_8spp.png",
		    "textures/blue_noise/scrambling_ranking_128x128_2d_16spp.png",
		    "textures/blue_noise/scrambling_ranking_128x128_2d_32spp.png",
		    "textures/blue_noise/scrambling_ranking_128x128_2d_64spp.png",
		    "textures/blue_noise/scrambling_ranking_128x128_2d_128spp.png",
		    "textures/blue_noise/scrambling_ranking_128x128_2d_256spp.png"};

		enum BlueNoiseSpp : uint8_t
		{
			Blue_Noise_1SPP,
			Blue_Noise_2SPP,
			Blue_Noise_4SPP,
			Blue_Noise_8SPP,
			Blue_Noise_16SPP,
			Blue_Noise_32SPP,
			Blue_Noise_64SPP,
			Blue_Noise_128SPP,
			Length
		};

		namespace global::component
		{
			struct BlueNoise
			{
				Texture2D::Ptr sobolSequence;
				Texture2D::Ptr scramblingRanking[BlueNoiseSpp::Length];
			};
		}

		auto registerBlueNoiseModule(std::shared_ptr<ExecutePoint> executePoint) -> void;
	}
}