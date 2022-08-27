//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "BlueNoise.h"
#include "Scene/System/ExecutePoint.h"

namespace maple
{
	namespace blue_noise
	{
		auto registerBlueNoiseModule(std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerGlobalComponent<blue_noise::global::component::BlueNoise>([](auto &noise) {
				noise.sobolSequence = Texture2D::create(blue_noise::SOBOL_TEXTURE, blue_noise::SOBOL_TEXTURE);
				for (int32_t i = 0; i < blue_noise ::BlueNoiseSpp::Length; i++)
				{
					noise.scramblingRanking[i] = Texture2D::create(blue_noise::SCRAMBLING_RANKING_TEXTURES[i], blue_noise::SCRAMBLING_RANKING_TEXTURES[i]);
				}
			});
		}
	}        // namespace blue_noise
}        // namespace maple