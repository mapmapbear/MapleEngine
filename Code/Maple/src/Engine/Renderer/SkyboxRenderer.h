//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Engine/Core.h"
#include "Renderer.h"
#include "Scene/System/ExecutePoint.h"
#include <IconsMaterialDesignIcons.h>
namespace maple
{
	class PrefilterRenderer;

	namespace component
	{
		struct SkyboxData
		{
			struct UniformBufferObject
			{
				glm::mat4 invProj;
				glm::mat4 invView;
				glm::vec4 skyColorBottom;
				glm::vec4 skyColorTop;
				glm::vec4 lightDirection;
				glm::vec4 viewPos;
			};

			std::shared_ptr<Shader>        skyboxShader;
			std::shared_ptr<Pipeline>      pipeline;
			std::shared_ptr<DescriptorSet> descriptorSet;
			std::shared_ptr<Mesh>          skyboxMesh;

			bool pseudoSky = false;

			std::shared_ptr<Shader>        pseudoSkyshader;
			std::shared_ptr<DescriptorSet> pseudoSkydescriptorSet;
			std::shared_ptr<Mesh>          screenMesh;

			UniformBufferObject skyUniformObject;

			std::shared_ptr<Texture> skybox;
			std::shared_ptr<Texture> environmentMap;
			std::shared_ptr<Texture> irradianceMap;

			glm::mat4 projView = glm::mat4(1);

			std::shared_ptr<PrefilterRenderer> prefilterRenderer;

			int32_t cubeMapMode  = 0;
			float   cubeMapLevel = 0;
		};
	}        // namespace component

	namespace skybox_renderer
	{
		auto registerSkyboxRenderer(ExecuteQueue &begin, ExecuteQueue &renderer, std::shared_ptr<ExecutePoint> executePoint) -> void;
	};
}        // namespace maple
