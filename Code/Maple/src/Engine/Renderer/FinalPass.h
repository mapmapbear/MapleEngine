//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <memory>
#include "Scene/System/ExecutePoint.h"

namespace maple
{
	class Shader;
	class DescriptorSet;
	class Texture;

	namespace component
	{
		struct FinalPass
		{
			std::shared_ptr<Shader>        finalShader;
			std::shared_ptr<DescriptorSet> finalDescriptorSet;
			std::shared_ptr<Texture> renderTarget;

			FinalPass();
		};
	}

	namespace final_screen_pass
	{
		auto registerFinalPass( ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> executePoint) -> void;
	};
}        // namespace maple
