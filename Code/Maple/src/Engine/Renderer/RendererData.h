//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Math/Frustum.h"
#include <glm/glm.hpp>
namespace maple
{
	class Transform;

	namespace component
	{
		struct CameraView
		{
			glm::mat4  proj;
			glm::mat4  view;
			glm::mat4  projView;
			glm::mat4  projViewOld;
			float      nearPlane;
			float      farPlane;
			Frustum    frustum;
			Transform *cameraTransform;
		};

		struct EnvironmentData
		{
			std::shared_ptr<Texture> skybox;
			std::shared_ptr<Texture> environmentMap;
			std::shared_ptr<Texture> irradianceMap;
			bool	cloud = false;
		};

		struct RendererData
		{
			CommandBuffer* commandBuffer = nullptr;
			GBuffer* gbuffer = nullptr;
			std::shared_ptr<Mesh> screenQuad;
		};
	}        // namespace component

#ifdef MAPLE_OPENGL
	constexpr glm::mat4 BIAS_MATRIX = {
	    0.5, 0.0, 0.0, 0.0,
	    0.0, 0.5, 0.0, 0.0,
	    0.0, 0.0, 0.5, 0.0,
	    0.5, 0.5, 0.5, 1.0};
#endif        // MAPLE_OPENGL

#ifdef MAPLE_VULKAN
	constexpr glm::mat4 BIAS_MATRIX = {
	    0.5, 0.0, 0.0, 0.0,
	    0.0, 0.5, 0.0, 0.0,
	    0.0, 0.0, 1.0, 0.0,
	    0.5, 0.5, 0.0, 1.0};
#endif
}        // namespace maple
