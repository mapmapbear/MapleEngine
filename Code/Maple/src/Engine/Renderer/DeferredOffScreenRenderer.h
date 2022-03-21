//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Renderer.h"

#include "Engine/Material.h"
#include "Engine/Mesh.h"
#include "RHI/DescriptorSet.h"
#include "RHI/Pipeline.h"
#include "RHI/Shader.h"
#include "RHI/Texture.h"
#include "Scene/System/ExecutePoint.h"

#include <memory>

namespace maple
{
	namespace component
	{
		struct DeferredData
		{
			std::vector<RenderCommand>                  commandQueue;
			std::shared_ptr<Material>                   defaultMaterial;
			std::vector<std::shared_ptr<DescriptorSet>> descriptorColorSet;
			std::vector<std::shared_ptr<DescriptorSet>> descriptorLightSet;
			std::shared_ptr<DescriptorSet>				descriptorAnim;

			std::shared_ptr<Texture2D> preintegratedFG;
			std::shared_ptr<Shader> deferredColorShader;        //stage 0 get all color information
			std::shared_ptr<Shader> deferredColorAnimShader;   
			std::shared_ptr<Shader> deferredLightShader;        //stage 1 process lighting
			std::shared_ptr<Shader> stencilShader;

			std::shared_ptr<DescriptorSet> stencilDescriptorSet;

			std::shared_ptr<Mesh>     screenQuad;

			bool depthTest = true;

			DeferredData();
		};
	}        // namespace component

	namespace deferred_offscreen
	{
		auto registerDeferredOffScreenRenderer(ExecuteQueue &begin, ExecuteQueue &renderer, std::shared_ptr<ExecutePoint> executePoint) -> void;
	};

	namespace deferred_lighting
	{
		auto registerDeferredLighting(ExecuteQueue& begin, ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> executePoint) -> void;
	};
}        // namespace maple
