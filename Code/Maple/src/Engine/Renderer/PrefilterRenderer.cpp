//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "PrefilterRenderer.h"

#include "RHI/CommandBuffer.h"
#include "RHI/DescriptorSet.h"
#include "RHI/FrameBuffer.h"
#include "RHI/Pipeline.h"
#include "RHI/RenderPass.h"
#include "RHI/Shader.h"
#include "RHI/Texture.h"
#include "RHI/UniformBuffer.h"

#include "Scene/Component/Environment.h"

#include "Application.h"
#include "Engine/Camera.h"
#include "Engine/GBuffer.h"
#include "Engine/Mesh.h"
#include "Engine/CaptureGraph.h"

#include "FileSystem/File.h"
#include "Scene/Scene.h"

#include <glm/gtc/type_ptr.hpp>

#define _USE_MATH_DEFINES
#include <math.h>

namespace maple
{
	const glm::mat4 captureViews[] =
	    {
	        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
	        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
	        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
	        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
	        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
	        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};

	const glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

	const glm::mat4 captureProjView[] =
	    {
	        projection * captureViews[0],
	        projection *captureViews[1],
	        projection *captureViews[2],
	        projection *captureViews[3],
	        projection *captureViews[4],
	        projection *captureViews[5]};

	PrefilterRenderer::PrefilterRenderer()
	{
	}

	PrefilterRenderer::~PrefilterRenderer()
	{
	}

	auto PrefilterRenderer::init() -> void
	{
		cubeMapShader    = Shader::create("shaders/CubeMap.shader");
		irradianceShader = Shader::create("shaders/Irradiance.shader");
		prefilterShader  = Shader::create("shaders/Prefilter.shader");

		skyboxCube = TextureCube::create(SkyboxSize, TextureFormat::RGBA32, 5);

		skyboxCaptureColor = Texture2D::create(SkyboxSize, SkyboxSize, nullptr, {}, {false, false, false});

		skyboxCaptureColor->buildTexture(
		    TextureFormat::RGBA32,
		    SkyboxSize,
		    SkyboxSize,
		    false, false, false);

		irradianceCaptureColor = Texture2D::create();
		irradianceCaptureColor->buildTexture(
		    TextureFormat::RGBA32,
		    component::Environment::IrradianceMapSize,
		    component::Environment::IrradianceMapSize,
		    false, false, false);

		prefilterCaptureColor = Texture2D::create();
		prefilterCaptureColor->buildTexture(
		    TextureFormat::RGBA32,
		    component::Environment::PrefilterMapSize,
		    component::Environment::PrefilterMapSize,
		    false, false, false);

		cube = Mesh::createCube();

		cubeMapSet = DescriptorSet::create({0, cubeMapShader.get()});

		irradianceSet = DescriptorSet::create({0, irradianceShader.get()});

		prefilterSet = DescriptorSet::create({0, prefilterShader.get()});

		skyboxDepth = TextureDepth::create(SkyboxSize, SkyboxSize);
		createPipeline();
		updateUniform();
	}

	auto PrefilterRenderer::renderScene(capture_graph::component::RenderGraph& graph) -> void
	{
		if (envComponent == nullptr)
		{
			return;
		}

		if (!envComponent->pseudoSky)
		{
			if (envComponent && envComponent->environment == nullptr)
			{
				envComponent->environment = skyboxCube;
			}

			generateSkybox(graph);

			updateIrradianceDescriptor();
			generateIrradianceMap(graph);

			updatePrefilterDescriptor();
			generatePrefilterMap(graph);

			envComponent = nullptr;
		}
	}

	auto PrefilterRenderer::present() -> void
	{
	}

	auto PrefilterRenderer::beginScene(component::Environment & env) -> void
	{
		if (equirectangularMap != env.equirectangularMap && !env.pseudoSky)
		{
			equirectangularMap = env.equirectangularMap;
			envComponent = &env;
			updateUniform();
		}
	}

	auto PrefilterRenderer::updateIrradianceDescriptor() -> void
	{
		if (envComponent)
		{
			irradianceSet->setTexture("uCubeMapSampler", skyboxCube);
			irradianceSet->update();
		}
	}

	auto PrefilterRenderer::updatePrefilterDescriptor() -> void
	{
		if (envComponent)
		{
			prefilterSet->setTexture("uCubeMapSampler", skyboxCube);
			prefilterSet->update();
		}
	}

	auto PrefilterRenderer::generateSkybox(capture_graph::component::RenderGraph& graph) -> void
	{
		
		const auto proj         = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
		cubeMapSet->setUniform("UniformBufferObject", "proj", glm::value_ptr(proj));

		auto cmd = Application::getGraphicsContext()->getSwapChain()->getCurrentCommandBuffer();
		if (equirectangularMap)
		{
			cubeMapSet->setTexture("equirectangularMap", equirectangularMap);
		}
		cubeMapSet->update();

		PipelineInfo pipeInfo;
		pipeInfo.shader              = cubeMapShader;
		pipeInfo.cullMode            = CullMode::Front;
		pipeInfo.transparencyEnabled = false;
		pipeInfo.depthBiasEnabled    = false;
		pipeInfo.clearTargets        = true;
		pipeInfo.colorTargets[0]     = skyboxCaptureColor;
		pipeInfo.colorTargets[1]     = skyboxCube;
		auto cubeMapPipeline = Pipeline::get(pipeInfo, { cubeMapSet },graph);

		for (auto faceId = 0; faceId < 6; faceId++)
		{
			auto  framebuffer = cubeMapPipeline->bind(cmd, 0, faceId);
			auto &constants   = cubeMapShader->getPushConstants();
			if (constants.size() > 0)
			{
				constants[0].setValue("view", glm::value_ptr(captureViews[faceId]));
				cubeMapShader->bindPushConstants(cmd, cubeMapPipeline.get());
			}
			Application::getRenderDevice()->bindDescriptorSets(cubeMapPipeline.get(), cmd, 0, {cubeMapSet});
			Renderer::drawMesh(cmd, cubeMapPipeline.get(), cube.get());
			cubeMapPipeline->end(cmd);
			skyboxCube->update(cmd, framebuffer, faceId);
		}
		skyboxCube->generateMipmap(cmd);
	}

	auto PrefilterRenderer::generateIrradianceMap(capture_graph::component::RenderGraph& graph) -> void
	{
		PipelineInfo pipeInfo;
		pipeInfo.shader   = irradianceShader;
		pipeInfo.cullMode = CullMode::None;

		pipeInfo.transparencyEnabled = false;
		pipeInfo.depthBiasEnabled    = false;
		pipeInfo.clearTargets        = true;

		pipeInfo.colorTargets[0] = irradianceCaptureColor;
		pipeInfo.colorTargets[1] = envComponent->irradianceMap;

		auto pipeline = Pipeline::get(pipeInfo,{ irradianceSet }, graph);

		auto cmd = Application::getGraphicsContext()->getSwapChain()->getCurrentCommandBuffer();

		for (auto faceId = 0; faceId < 6; faceId++)
		{
			auto  fb        = pipeline->bind(cmd, 0, faceId);
			auto &constants = irradianceShader->getPushConstants();

			constants[0].setValue("projView", glm::value_ptr(captureProjView[faceId]));
			irradianceShader->bindPushConstants(cmd, pipeline.get());

			Renderer::bindDescriptorSets(pipeline.get(), cmd, 0, {irradianceSet});
			Renderer::drawMesh(cmd, pipeline.get(), cube.get());

			pipeline->end(cmd);
			envComponent->irradianceMap->update(cmd, fb, faceId);
		}
	}

	auto PrefilterRenderer::generatePrefilterMap(capture_graph::component::RenderGraph & graph) -> void
	{
		cubeMapSet->update();

		PipelineInfo pipeInfo;
		pipeInfo.shader   = prefilterShader;
		pipeInfo.cullMode = CullMode::None;

		pipeInfo.transparencyEnabled = false;
		pipeInfo.depthBiasEnabled    = false;
		pipeInfo.clearTargets        = true;

		pipeInfo.colorTargets[0] = prefilterCaptureColor;
		pipeInfo.colorTargets[1] = envComponent->prefilteredEnvironment;

		auto pipeline = Pipeline::get(pipeInfo, { prefilterSet }, graph);

		auto cmd = Application::getGraphicsContext()->getSwapChain()->getCurrentCommandBuffer();

		auto maxMips = 5;// std::pow(component::Environment::PrefilterMapSize, 2);

		for (auto mip = 0; mip < maxMips; ++mip)
		{
			auto roughness = (float) mip / (float)(maxMips - 1);

			prefilterSet->setUniform("UniformBufferRoughness", "constRoughness", &roughness, true);

			for (auto faceId = 0; faceId < 6; faceId++)
			{
				auto fb = pipeline->bind(cmd, 0, faceId, mip);

				auto &constants = prefilterShader->getPushConstants();
				constants[0].setValue("projView", glm::value_ptr(captureProjView[faceId]));
				prefilterShader->bindPushConstants(cmd, pipeline.get());

				Application::getRenderDevice()->bindDescriptorSets(pipeline.get(), cmd, 0, {prefilterSet});
				Renderer::drawMesh(cmd, pipeline.get(), cube.get());

				pipeline->end(cmd);
				envComponent->prefilteredEnvironment->update(cmd, fb, faceId, mip);
			}
		}
	}

	auto PrefilterRenderer::createPipeline() -> void
	{
	}

	auto PrefilterRenderer::updateUniform() -> void
	{
		auto proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

		cubeMapSet->setUniform("UniformBufferObject", "proj", glm::value_ptr(proj));

		if (equirectangularMap)
		{
			cubeMapSet->setTexture("equirectangularMap", equirectangularMap);
		}
	}

	auto PrefilterRenderer::createFrameBuffer() -> void
	{
	}
};        // namespace maple
