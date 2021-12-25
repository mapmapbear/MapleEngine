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

#include "Application.h"
#include "Engine/Camera.h"
#include "Engine/GBuffer.h"
#include "Engine/Mesh.h"
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

	const glm::mat4 captureProjView[] =
	    {
	        glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f) * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
	        glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f) * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
	        glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f) * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
	        glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f) * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
	        glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f) * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
	        glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f) * glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};

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

		skyboxCaptureColor = Texture2D::create(SkyboxSize, SkyboxSize, nullptr, {}, {false, false, false});
		skyboxCube         = TextureCube::create(SkyboxSize, TextureFormat::RGBA32, 5);

		irradianceCaptureColor = Texture2D::create();
		irradianceCaptureColor->buildTexture(
		    TextureFormat::RGBA32,
		    Environment::IrradianceMapSize,
		    Environment::IrradianceMapSize,
		    true, false, false);

		prefilterCaptureColor = Texture2D::create();
		prefilterCaptureColor->buildTexture(
		    TextureFormat::RGBA32,
		    Environment::PrefilterMapSize,
		    Environment::PrefilterMapSize,
		    true, false, false);

		cube = Mesh::createCube();

		DescriptorInfo info;
		info.layoutIndex = 0;

		info.shader = cubeMapShader.get();
		cubeMapSet  = DescriptorSet::create(info);

		info.shader   = irradianceShader.get();
		irradianceSet = DescriptorSet::create(info);

		info.shader  = prefilterShader.get();
		prefilterSet = DescriptorSet::create(info);

		skyboxDepth = TextureDepth::create(SkyboxSize, SkyboxSize);
		createPipeline();
		updateUniform();
	}

	auto PrefilterRenderer::begin() -> void
	{
	}

	auto PrefilterRenderer::end() -> void
	{
	}

	auto PrefilterRenderer::renderScene() -> void
	{
		if (envComponent == nullptr)
		{
			return;
		}

		if (envComponent && envComponent->getEnvironment() == nullptr)
		{
			envComponent->setEnvironmnet(skyboxCube);
		}

		generateSkybox();

		updateIrradianceDescriptor();
		generateIrradianceMap();

		updatePrefilterDescriptor();
		generatePrefilterMap();

		envComponent = nullptr;
	}

	auto PrefilterRenderer::present() -> void
	{
	}

	auto PrefilterRenderer::beginScene(Scene *scene) -> void
	{
		auto &registry = scene->getRegistry();
		auto  view     = registry.view<Environment>();
		if (!view.empty())
		{
			auto env = &view.get<Environment>(view.front());
			if (equirectangularMap != env->getEquirectangularMap())
			{
				equirectangularMap = env->getEquirectangularMap();
				envComponent       = env;
				updateUniform();
			}
		}
	}

	auto PrefilterRenderer::updateIrradianceDescriptor() -> void
	{
		if (envComponent)
		{
			irradianceSet->setTexture("uCubeMapSampler", envComponent->getEnvironment());
			irradianceSet->update();
		}
	}

	auto PrefilterRenderer::updatePrefilterDescriptor() -> void
	{
		if (envComponent)
		{
			prefilterSet->setTexture("uCubeMapSampler", envComponent->getEnvironment());
			irradianceSet->update();
		}
	}

	auto PrefilterRenderer::generateSkybox() -> void
	{
		const auto maxMipLevels = 5;
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
		auto cubeMapPipeline              = Pipeline::get(pipeInfo);

		for (auto faceId = 0; faceId < 6; faceId++)
		{
			cubeMapPipeline->bind(cmd, 0, faceId);
			auto &constants = cubeMapShader->getPushConstants();
			if (constants.size() > 0)
			{
				constants[0].setValue("view", glm::value_ptr(captureViews[faceId]));
				cubeMapShader->bindPushConstants(cmd, cubeMapPipeline.get());
			}
			Application::getRenderDevice()->bindDescriptorSets(cubeMapPipeline.get(), cmd, 0, {cubeMapSet});
			Renderer::drawMesh(cmd, cubeMapPipeline.get(), cube.get());
		}
		skyboxCube->generateMipmap();
		cubeMapPipeline->end(cmd);
	}

	auto PrefilterRenderer::generateIrradianceMap() -> void
	{
		PipelineInfo pipeInfo;
		pipeInfo.shader   = irradianceShader;
		pipeInfo.cullMode = CullMode::None;

		pipeInfo.transparencyEnabled = false;
		pipeInfo.depthBiasEnabled    = false;
		pipeInfo.clearTargets        = false;

		pipeInfo.colorTargets[0] = irradianceCaptureColor;
		pipeInfo.colorTargets[1] = envComponent->getIrradianceMap();

		auto pipeline = Pipeline::get(pipeInfo);

		auto cmd = Application::getGraphicsContext()->getSwapChain()->getCurrentCommandBuffer();

		for (auto faceId = 0; faceId < 6; faceId++)
		{
			pipeline->bind(cmd, 0, faceId);

			irradianceSet->setUniformBufferData("UniformBufferObject", &captureProjView[faceId]);
			irradianceSet->update();

			Application::getRenderDevice()->bindDescriptorSets(pipeline.get(), cmd, 0, {irradianceSet});
			Renderer::drawMesh(cmd, pipeline.get(), cube.get());
		}

		pipeline->end(cmd);
	}

	auto PrefilterRenderer::generatePrefilterMap() -> void
	{
		cubeMapSet->update();

		PipelineInfo pipeInfo;
		pipeInfo.shader   = prefilterShader;
		pipeInfo.cullMode = CullMode::None;

		pipeInfo.transparencyEnabled = false;
		pipeInfo.depthBiasEnabled    = false;
		pipeInfo.clearTargets        = false;

		pipeInfo.colorTargets[0] = prefilterCaptureColor;
		pipeInfo.colorTargets[1] = envComponent->getPrefilteredEnvironment();

		auto pipeline = Pipeline::get(pipeInfo);

		auto cmd = Application::getGraphicsContext()->getSwapChain()->getCurrentCommandBuffer();

		const auto maxMipLevels = 5;
		for (auto mip = 0; mip < maxMipLevels; ++mip)
		{
			auto roughness = (float) mip / (float) (maxMipLevels - 1);
			prefilterSet->setUniformBufferData("UniformBufferRoughness", &roughness);
			for (auto faceId = 0; faceId < 6; faceId++)
			{
				pipeline->bind(cmd, 0, faceId, mip);

				prefilterSet->setUniformBufferData("UniformBufferObject", &captureProjView[faceId]);
				prefilterSet->update();

				Application::getRenderDevice()->bindDescriptorSets(pipeline.get(), cmd, 0, {prefilterSet});
				Renderer::drawMesh(cmd, pipeline.get(), cube.get());
			}
		}

		pipeline->end(cmd);
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
