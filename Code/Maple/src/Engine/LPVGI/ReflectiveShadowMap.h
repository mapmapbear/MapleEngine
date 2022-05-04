//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Engine/Renderer/Renderer.h"
#include "Engine/Core.h"
#include "Math/Frustum.h"
#include "RHI/Shader.h"
#include "Scene/System/ExecutePoint.h"

namespace maple
{
	namespace component
	{
		struct ReflectiveShadowData
		{
			constexpr static int32_t SHADOW_SIZE = 256;

			bool                                        enable = false;
			std::shared_ptr<Shader>                     shader;
			std::vector<std::shared_ptr<DescriptorSet>> descriptorSets;
			std::shared_ptr<Texture2D>                  fluxTexture;
			std::shared_ptr<Texture2D>                  worldTexture;
			std::shared_ptr<Texture2D>                  normalTexture;
			std::shared_ptr<TextureDepth>               fluxDepth;
			std::vector<RenderCommand>				    commandQueue;
			Frustum										frustum;
			glm::mat4									projView;
			glm::mat4									lightMatrix;
			float										lightArea = 1.0f;
		};
	}        // namespace component

	namespace reflective_shadow_map
	{
		auto MAPLE_EXPORT registerGlobalComponent(std::shared_ptr<ExecutePoint> executePoint) -> void;
		auto registerShadowMap(ExecuteQueue& begin, ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> executePoint) -> void;
	};
};        // namespace maple
