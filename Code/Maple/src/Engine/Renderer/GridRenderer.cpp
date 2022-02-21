//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "GridRenderer.h"

#include "RHI/CommandBuffer.h"
#include "RHI/FrameBuffer.h"
#include "RHI/GraphicsContext.h"
#include "RHI/IndexBuffer.h"
#include "RHI/Pipeline.h"
#include "RHI/RenderPass.h"
#include "RHI/Shader.h"
#include "RHI/SwapChain.h"
#include "RHI/Texture.h"
#include "RHI/UniformBuffer.h"
#include "RHI/VertexBuffer.h"

#include "Engine/Camera.h"
#include "Engine/GBuffer.h"
#include "Engine/Mesh.h"
#include "Engine/Profiler.h"

#include "Scene/Component/Transform.h"
#include "Scene/Scene.h"

#include "RendererData.h"

#include "ImGui/ImGuiHelpers.h"

#include <glm/gtc/type_ptr.hpp>

#include "Application.h"

#include <ecs/ecs.h>

namespace maple
{
	namespace component 
	{
		struct GridData
		{
			std::shared_ptr<Mesh> quad;
			std::shared_ptr<Shader> gridShader;
			std::shared_ptr<DescriptorSet> descriptorSet;

			struct UniformBufferObject
			{
				glm::vec4 cameraPos;
				float     scale = 500.0f;
				float     res = 1.4f;
				float     maxDistance = 600.0f;
				float     p1;
			} systemBuffer;

			GridData() 
			{
				gridShader = Shader::create("shaders/Grid.shader");
				quad = Mesh::createPlane(500, 500, maple::UP);
				descriptorSet = DescriptorSet::create({ 0, gridShader.get() });
			}
		};
	}

	namespace on_begin
	{
		using Entity = ecs::Chain
			::Read<component::RendererData>
			::Write<component::GridData>
			::Write<component::CameraView>
			::To<ecs::Entity>;

		inline auto system(Entity entity,ecs::World world)
		{
			auto [render, grid, camera] = entity;
			grid.descriptorSet->setUniformBufferData("UniformBufferObject", glm::value_ptr(camera.projView));
			grid.systemBuffer.cameraPos = glm::vec4(camera.cameraTransform->getWorldPosition(), 1.f);
			grid.descriptorSet->setUniformBufferData("UniformBuffer", &grid.systemBuffer);
		}
	}

	namespace on_render 
	{
		using Entity = ecs::Chain
			::Read<component::GridData>
			::Read<component::RendererData>
			::To<ecs::Entity>;
		inline auto system(Entity entity, ecs::World world)
		{
			auto [grid,render] = entity;

			PipelineInfo pipeInfo;
			pipeInfo.shader = grid.gridShader;
			pipeInfo.cullMode = CullMode::None;
			pipeInfo.transparencyEnabled = true;
			pipeInfo.depthBiasEnabled = false;
			pipeInfo.clearTargets = false;
			pipeInfo.depthTarget = render.gbuffer->getDepthBuffer();
			pipeInfo.colorTargets[0] = render.gbuffer->getBuffer(GBufferTextures::SCREEN);
			pipeInfo.blendMode = BlendMode::SrcAlphaOneMinusSrcAlpha;
			auto pipeline = Pipeline::get(pipeInfo);

			pipeline->bind(render.commandBuffer);
			grid.descriptorSet->update();
			Renderer::bindDescriptorSets(pipeline.get(), render.commandBuffer, 0, { grid.descriptorSet });
			Renderer::drawMesh(render.commandBuffer, pipeline.get(), grid.quad.get());
			pipeline->end(render.commandBuffer);
		}
	}

	namespace grid_renderer
	{
		auto registerGridRenderer(ExecuteQueue& begin, ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerGlobalComponent<component::GridData>();
			executePoint->registerWithinQueue<on_begin::system>(begin);
			executePoint->registerWithinQueue<on_render::system>(renderer);
		}
	};

};        // namespace maple
