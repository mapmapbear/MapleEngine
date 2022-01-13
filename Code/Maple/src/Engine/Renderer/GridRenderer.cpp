//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "GridRenderer.h"
#include "FileSystem/File.h"
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

#include "Engine/GBuffer.h"
#include "Engine/Mesh.h"

#include "Engine/Camera.h"
#include "Scene/Component/Transform.h"

#include "Scene/Scene.h"

#include "Application.h"

#include "Engine/Profiler.h"

#include <glm/gtc/type_ptr.hpp>
namespace maple
{
	GridRenderer::GridRenderer(uint32_t width, uint32_t height)
	{
		this->width  = width;
		this->height = height;
	}

	GridRenderer::~GridRenderer()
	{
	}

	auto GridRenderer::init(const std::shared_ptr<GBuffer> &buffer) -> void
	{
		this->gbuffer = buffer;
		gridShader    = Shader::create("shaders/Grid.shader");
		quad          = Mesh::createPlane(500, 500, maple::UP);
		descriptorSet = DescriptorSet::create({0, gridShader.get()});
	}

	auto GridRenderer::renderScene() -> void
	{
		PROFILE_FUNCTION();

		if (pipeline == nullptr && renderTexture)
		{
			PipelineInfo pipeInfo;
			pipeInfo.shader              = gridShader;
			pipeInfo.cullMode            = CullMode::None;
			pipeInfo.transparencyEnabled = true;
			pipeInfo.depthBiasEnabled    = false;
			pipeInfo.clearTargets        = false;
			pipeInfo.depthTarget         = gbuffer->getDepthBuffer();
			pipeInfo.colorTargets[0]     = gbuffer->getBuffer(GBufferTextures::SCREEN);
			pipeInfo.blendMode           = BlendMode::SrcAlphaOneMinusSrcAlpha;
			pipeline                     = Pipeline::get(pipeInfo);
		}

		if (pipeline)
		{
			pipeline->bind(getCommandBuffer());
			descriptorSet->update();
			bindDescriptorSets(pipeline.get(), getCommandBuffer(), 0, {descriptorSet});
			drawMesh(getCommandBuffer(), pipeline.get(), quad.get());
			pipeline->end(getCommandBuffer());
		}
	}

	auto GridRenderer::renderPreviewScene() -> void
	{
	}

	auto GridRenderer::beginScene(Scene *scene, const glm::mat4 &projView) -> void
	{
		PROFILE_FUNCTION();
		auto camera = scene->getCamera();

		if (camera.first == nullptr || camera.second == nullptr || pipeline == nullptr)
		{
			return;
		}

		descriptorSet->setUniformBufferData("UniformBufferObject", glm::value_ptr(projView));
		systemBuffer.cameraPos = glm::vec4(camera.second->getWorldPosition(), 1.f);
		descriptorSet->setUniformBufferData("UniformBuffer", &systemBuffer);
	}

	auto GridRenderer::beginScenePreview(Scene *scene, const glm::mat4 &projView) -> void
	{
		PROFILE_FUNCTION();
	}

	auto GridRenderer::onResize(uint32_t width, uint32_t height) -> void
	{
		this->width  = width;
		this->height = height;
	}

	auto GridRenderer::setRenderTarget(std::shared_ptr<Texture> texture, bool rebuildFramebuffer) -> void
	{
		PROFILE_FUNCTION();
		Renderer::setRenderTarget(texture, rebuildFramebuffer);
		pipeline = nullptr;
	}
};        // namespace maple
