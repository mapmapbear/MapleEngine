//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "Noise.h"
#define STB_PERLIN_IMPLEMENTATION
#include <stb_perlin.h>

namespace maple
{
	namespace Noise
	{
		auto perlinNoise(int32_t x, int32_t y) -> float
		{
			const int32_t offsetx = 100;
			const int32_t offsety = -50;
			const float   layer1  = 25.0f;
			const float   layer2  = 180.0f;

			float xx = float(x + offsetx);
			float yy = float(y + offsety);
			return (((stb_perlin_noise3(xx / layer1, yy / layer1, 0, 0, 0, 0) + 1.0f) / 2.0f) +
			        ((stb_perlin_noise3(xx / layer2, yy / layer2, 0, 0, 0, 0) + 1.0f) / 2.0f)) /
			       2.0f;
		}
	};        // namespace Noise
};            // namespace maple