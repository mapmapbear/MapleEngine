//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <memory>
#include <IconsMaterialDesignIcons.h>
#include "Scene/System/ExecutePoint.h"

namespace maple
{
	class Shader;
	class DescriptorSet;
	class Texture;

	namespace component
	{
		struct MAPLE_EXPORT FinalPass
		{
			constexpr static char* ICON = ICON_MDI_SCREEN_ROTATION;

			std::shared_ptr<Shader>        finalShader;
			std::shared_ptr<DescriptorSet> finalDescriptorSet;
			std::shared_ptr<Texture> renderTarget;
			float exposure = 1.0;
			int32_t toneMapIndex = 1;
			FinalPass();
		};
	}

	namespace final_screen_pass
	{
		auto registerFinalPass( ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> executePoint) -> void;
	};
}        // namespace maple
