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

	namespace component
	{
		struct BloomData
		{
			bool                           enable = false;
			std::shared_ptr<DescriptorSet> bloomDescriptorSet;
			std::shared_ptr<Shader>        bloomShader;
			float                          blurScale    = 0.003f;
			float                          blurStrength = 1.5f;
		};

		struct SSRData
		{
			bool                           enable = false;
			std::shared_ptr<DescriptorSet> ssrDescriptorSet;
			std::shared_ptr<Shader>        ssrShader;
		};
	};        // namespace component

	namespace post_process
	{
		auto registerSSR(ExecuteQueue &renderer, std::shared_ptr<ExecutePoint> executePoint) -> void;
		auto registerBloom(ExecuteQueue &renderer, std::shared_ptr<ExecutePoint> executePoint) -> void;
	};        // namespace post_process
}        // namespace maple
