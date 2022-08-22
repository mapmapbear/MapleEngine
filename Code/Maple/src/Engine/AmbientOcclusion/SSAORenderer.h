//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Scene/System/ExecutePoint.h"
#include <memory>

namespace maple
{
	class Shader;
	class DescriptorSet;

	namespace ssao
	{
		namespace component
		{
			struct SSAOData
			{
				std::shared_ptr<Shader>                     ssaoShader;
				std::shared_ptr<Shader>                     ssaoBlurShader;
				std::vector<std::shared_ptr<DescriptorSet>> ssaoSet;
				std::vector<std::shared_ptr<DescriptorSet>> ssaoBlurSet;
				bool                                        enable     = false;
				float                                       bias       = 0.025;
				float                                       ssaoRadius = 0.25f;
			};
		}        // namespace component

		auto registerSSAOPass(ExecuteQueue &begin, ExecuteQueue &renderer, std::shared_ptr<ExecutePoint> executePoint) -> void;
	}
}        // namespace maple