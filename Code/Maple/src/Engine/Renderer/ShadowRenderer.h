//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Engine/Core.h"
#include "Engine/Renderer/Renderer.h"
#include "Math/Frustum.h"
#include "RHI/Shader.h"
#include "Scene/System/ExecutePoint.h"

namespace maple
{
	namespace ShadowingMethod
	{
		enum Id
		{
			ShadowMap,
			TraceShadowCone,
			Length
		};

		static const char *Names[] =
		    {
		        "ShadowMap",
		        "TraceShadowCone",
		        nullptr};
	}        // namespace ShadowingMethod

	namespace component
	{
		struct ShadowMapData
		{
			float cascadeSplitLambda    = 0.995f;
			float sceneRadiusMultiplier = 1.4f;
			float lightSize             = 1.5f;
			float maxShadowDistance     = 400.0f;
			float shadowFade            = 40.0f;
			float cascadeTransitionFade = 3.0f;
			float initialBias           = 0.005f;
			bool  shadowMapsInvalidated = true;
			bool  dirty                 = true;

			maple::ShadowingMethod::Id shadowMethod = ShadowingMethod::ShadowMap;

			uint32_t  shadowMapNum                  = 4;
			uint32_t  shadowMapSize                 = SHADOWMAP_SiZE_MAX;
			glm::mat4 shadowProjView[SHADOWMAP_MAX] = {};
			glm::vec4 splitDepth[SHADOWMAP_MAX]     = {};
			glm::mat4 lightMatrix                   = glm::mat4(1.f);
			glm::mat4 shadowProj                    = glm::mat4(1.f);
			glm::vec3 lightDir                      = {};
			Frustum   cascadeFrustums[SHADOWMAP_MAX];

			std::vector<std::shared_ptr<DescriptorSet>> descriptorSet;

			std::vector<std::shared_ptr<DescriptorSet>> animDescriptorSet;

			std::vector<RenderCommand>         cascadeCommandQueue[SHADOWMAP_MAX];
			std::vector<RenderCommand>         animationQueue;
			std::shared_ptr<Shader>            shader;
			std::shared_ptr<Shader>            animShader;
			std::shared_ptr<TextureDepthArray> shadowTexture;
		};
	}        // namespace component

	namespace shadow_map
	{
		auto registerShadowMap(ExecuteQueue &begin, ExecuteQueue &renderer, std::shared_ptr<ExecutePoint> executePoint) -> void;
	};
};        // namespace maple