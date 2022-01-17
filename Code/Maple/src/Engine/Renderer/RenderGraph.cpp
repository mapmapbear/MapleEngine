//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "RenderGraph.h"
#include "Application.h"
#include "Engine/Camera.h"
#include "Engine/GBuffer.h"
#include "Engine/Material.h"
#include "Engine/Mesh.h"
#include "Engine/Profiler.h"
#include "Engine/Quad2D.h"
#include "Engine/Vertex.h"

#include "RHI/CommandBuffer.h"
#include "RHI/GPUProfile.h"
#include "RHI/IndexBuffer.h"
#include "RHI/Pipeline.h"
#include "RHI/VertexBuffer.h"

#include "Scene/Component/Atmosphere.h"
#include "Scene/Component/Light.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Scene.h"

#include "Math/BoundingBox.h"
#include "Math/MathUtils.h"

#include "Window/NativeWindow.h"

#include "PostProcessRenderer.h"
#include "PrefilterRenderer.h"
#include "Renderer2D.h"

#include "Others/Randomizer.h"

#include <chrono>
#include <imgui/imgui.h>

namespace maple
{
	namespace
	{
		inline auto renderOutputMode(int32_t mode) -> const std::string
		{
			switch (mode)
			{
				case 0:
					return "Lighting";
				case 1:
					return "Color";
				case 2:
					return "Metallic";
				case 3:
					return "Roughness";
				case 4:
					return "AO";
				case 5:
					return "SSAO-Blur";
				case 6:
					return "Normal";
				case 7:
					return "Shadow Cascades";
				case 8:
					return "Depth";
				case 9:
					return "Position";
				case 10:
					return "PBR Sampler";
				case 11:
					return "SSAO ";
				case 12:
					return "Position - ViewSpace";
				case 13:
					return "Normal - ViewSpace";
				default:
					return "Lighting";
			}
		}

		inline auto cubeMapModeToString(int32_t mode) -> const std::string
		{
			switch (mode)
			{
				case 0:
					return "CubeMap";
				case 1:
					return "Prefilter";
				case 2:
					return "Irradiance";
			}
			return "";
		}

		inline auto ssaoKernel() -> const std::vector<glm::vec4>
		{
			constexpr int32_t      SSAO_KERNEL_SIZE = 64;
			std::vector<glm::vec4> ssaoKernels;
			for (auto i = 0; i < SSAO_KERNEL_SIZE; ++i)
			{
				glm::vec3 sample(
				    Randomizer::random() * 2.0 - 1.0,
				    Randomizer::random() * 2.0 - 1.0,
				    Randomizer::random());
				sample = glm::normalize(sample);
				sample *= Randomizer::random();
				float scale = float(i) / float(SSAO_KERNEL_SIZE);
				scale       = MathUtils::lerp(0.1f, 1.0f, scale * scale, false);
				ssaoKernels.emplace_back(glm::vec4(sample * scale, 0));
			}
			return ssaoKernels;
		}
	}        // namespace

#ifdef MAPLE_OPENGL
	constexpr glm::mat4 BIAS_MATRIX = {
	    0.5, 0.0, 0.0, 0.0,
	    0.0, 0.5, 0.0, 0.0,
	    0.0, 0.0, 0.5, 0.0,
	    0.5, 0.5, 0.5, 1.0};
#endif        // MAPLE_OPENGL

#if MAPLE_VULKAN
	constexpr glm::mat4 BIAS_MATRIX = {
	    0.5, 0.0, 0.0, 0.0,
	    0.0, 0.5, 0.0, 0.0,
	    0.0, 0.0, 1.0, 0.0,
	    0.5, 0.5, 0.0, 1.0};
#endif

	struct RenderGraph::ShadowData
	{
		float cascadeSplitLambda    = 0.995f;
		float sceneRadiusMultiplier = 1.4f;
		float lightSize             = 1.5f;
		float maxShadowDistance     = 400.0f;
		float shadowFade            = 40.0f;
		float cascadeTransitionFade = 3.0f;
		float initialBias           = 0.0023f;
		bool  shadowMapsInvalidated = true;

		uint32_t  shadowMapNum                   = 4;
		uint32_t  shadowMapSize                  = SHADOWMAP_SiZE_MAX;
		glm::mat4 shadowProjView[SHADOWMAP_MAX]  = {};
		glm::vec4 splitDepth[SHADOWMAP_MAX]      = {};
		glm::mat4 lightMatrix                    = glm::mat4(1.f);
		Frustum   cascadeFrustums[SHADOWMAP_MAX] = {};

		std::vector<std::shared_ptr<DescriptorSet>> descriptorSet;
		std::vector<std::shared_ptr<DescriptorSet>> currentDescriptorSets;

		std::vector<RenderCommand>         cascadeCommandQueue[SHADOWMAP_MAX];
		std::shared_ptr<Shader>            shader;
		std::shared_ptr<TextureDepthArray> shadowTexture;

		ShadowData()
		{
			shadowTexture = TextureDepthArray::create(SHADOWMAP_SiZE_MAX, SHADOWMAP_SiZE_MAX, shadowMapNum);
			shader        = Shader::create("shaders/Shadow.shader");

			DescriptorInfo createInfo{};
			createInfo.layoutIndex = 0;
			createInfo.shader      = shader.get();

			descriptorSet.resize(1);
			descriptorSet[0] = DescriptorSet::create(createInfo);
			currentDescriptorSets.resize(1);
			cascadeCommandQueue[0].reserve(500);
			cascadeCommandQueue[1].reserve(500);
			cascadeCommandQueue[2].reserve(500);
			cascadeCommandQueue[3].reserve(500);
		}
	};

	struct RenderGraph::ForwardData
	{
		std::shared_ptr<Texture>   defaultTexture;
		std::shared_ptr<Texture>   skybox;
		std::shared_ptr<Texture>   environmentMap;
		std::shared_ptr<Texture>   irradianceMap;
		std::shared_ptr<Texture>   renderTexture;
		std::shared_ptr<Texture>   depthTexture;
		std::shared_ptr<Material>  defaultMaterial;
		std::shared_ptr<Texture2D> preintegratedFG;
		std::vector<CommandBuffer> commandBuffers;
		std::vector<RenderCommand> commandQueue;

		std::vector<std::shared_ptr<DescriptorSet>> descriptorSet;

		std::shared_ptr<Shader> shader;

		Frustum   frustum;
		glm::vec4 clearColor      = {0.3f, 0.3f, 0.3f, 1.0f};
		glm::mat4 biasMatrix      = BIAS_MATRIX;
		uint32_t  renderMode      = 0;
		uint32_t  currentBufferID = 0;
		bool      depthTest       = true;
		float     cubeMapLevel    = 0;
		uint32_t  cubeMapMode     = 0;

		ForwardData()
		{
			shader = Shader::create("shaders/ForwardPBR.shader");
			commandQueue.reserve(1000);
			preintegratedFG = Texture2D::create("preintegrated", "textures/ibl_brdf_lut.png", {TextureFormat::RGBA8, TextureFilter::Linear, TextureFilter::Linear, TextureWrap::ClampToEdge});

			DescriptorInfo descriptorInfo{};
			descriptorInfo.shader = shader.get();
			descriptorSet.resize(3);

			descriptorInfo.layoutIndex = 0;
			descriptorSet[0]           = DescriptorSet::create(descriptorInfo);

			descriptorInfo.layoutIndex = 2;
			descriptorSet[2]           = DescriptorSet::create(descriptorInfo);

			MaterialProperties properties;
			properties.albedoColor       = glm::vec4(1.f, 1.f, 1.f, 1.f);
			properties.roughnessColor    = glm::vec4(0);
			properties.metallicColor     = glm::vec4(0);
			properties.usingAlbedoMap    = 0.0f;
			properties.usingRoughnessMap = 0.0f;
			properties.usingNormalMap    = 0.0f;
			properties.usingMetallicMap  = 0.0f;

			defaultMaterial = std::make_shared<Material>(shader, properties);
			defaultMaterial->createDescriptorSet();
			defaultMaterial->setRenderFlag(Material::RenderFlags::ForwardRender);
			defaultMaterial->removeRenderFlag(Material::RenderFlags::DeferredRender);
			defaultMaterial->setAlbedo(Texture2D::getDefaultTexture());

			irradianceMap  = TextureCube::create(1);
			environmentMap = irradianceMap;
		}
	};

	struct RenderGraph::DeferredData
	{
		std::vector<RenderCommand>                  commandQueue;
		std::shared_ptr<Texture>                    renderTexture;
		std::shared_ptr<Material>                   defaultMaterial;
		std::vector<std::shared_ptr<DescriptorSet>> descriptorColorSet;
		std::vector<std::shared_ptr<DescriptorSet>> descriptorLightSet;

		std::shared_ptr<Shader> deferredColorShader;        //stage 0 get all color information
		std::shared_ptr<Shader> deferredLightShader;        //stage 1 process lighting

		std::shared_ptr<Pipeline> deferredLightPipeline;

		std::shared_ptr<Mesh> screenQuad;

		bool depthTest = true;

		DeferredData()
		{
			deferredColorShader = Shader::create("shaders/DeferredColor.shader");
			deferredLightShader = Shader::create("shaders/DeferredLight.shader");

			commandQueue.reserve(1000);

			MaterialProperties properties;
			properties.albedoColor       = glm::vec4(1.f, 1.f, 1.f, 1.f);
			properties.roughnessColor    = glm::vec4(0);
			properties.metallicColor     = glm::vec4(0);
			properties.usingAlbedoMap    = 0.0f;
			properties.usingRoughnessMap = 0.0f;
			properties.usingNormalMap    = 0.0f;
			properties.usingMetallicMap  = 0.0f;

			defaultMaterial = std::make_shared<Material>(deferredColorShader, properties);
			defaultMaterial->createDescriptorSet();

			DescriptorInfo info{};
			descriptorColorSet.resize(3);
			descriptorLightSet.resize(1);

			info.shader           = deferredColorShader.get();
			info.layoutIndex      = 0;
			descriptorColorSet[0] = DescriptorSet::create(info);

			info.layoutIndex      = 2;
			descriptorColorSet[2] = DescriptorSet::create(info);

			info.shader           = deferredLightShader.get();
			info.layoutIndex      = 0;
			descriptorLightSet[0] = DescriptorSet::create(info);

			screenQuad = Mesh::createQuad(true);
		}
	};

	struct RenderGraph::SSAOData
	{
		std::shared_ptr<Shader>                     ssaoShader;
		std::shared_ptr<Shader>                     ssaoBlurShader;
		std::vector<std::shared_ptr<DescriptorSet>> ssaoSet;
		std::vector<std::shared_ptr<DescriptorSet>> ssaoBlurSet;

		bool  enable = true;
		float bias   = 0.025;

		SSAOData()
		{
			ssaoShader     = Shader::create("shaders/SSAO.shader");
			ssaoBlurShader = Shader::create("shaders/SSAOBlur.shader");

			DescriptorInfo info{};
			ssaoSet.resize(1);
			ssaoBlurSet.resize(1);

			info.shader      = ssaoShader.get();
			info.layoutIndex = 0;
			ssaoSet[0]       = DescriptorSet::create(info);

			info.shader      = ssaoBlurShader.get();
			info.layoutIndex = 0;
			ssaoBlurSet[0]   = DescriptorSet::create(info);

			auto ssao = ssaoKernel();
			ssaoSet[0]->setUniformBufferData("UBOSSAOKernel", ssao.data());
		}
	};

	struct RenderGraph::PreviewData
	{
		std::shared_ptr<Texture> renderTexture;
		std::shared_ptr<Texture> depthTexture;

		std::vector<RenderCommand>                  commandQueue;
		std::vector<std::shared_ptr<DescriptorSet>> descriporSets;
		std::shared_ptr<Material>                   defaultMaterial;
		std::shared_ptr<Shader>                     shader;

		PreviewData()
		{
			shader = Shader::create("shaders/ForwardPreview.shader");
			descriporSets.resize(3);
			descriporSets[0] = DescriptorSet::create({0, shader.get()});
			descriporSets[2] = DescriptorSet::create({2, shader.get()});

			MaterialProperties properties;
			properties.albedoColor       = glm::vec4(1.f, 1.f, 1.f, 1.f);
			properties.roughnessColor    = glm::vec4(0);
			properties.metallicColor     = glm::vec4(0);
			properties.usingAlbedoMap    = 1.0f;
			properties.usingRoughnessMap = 0.0f;
			properties.usingNormalMap    = 0.0f;
			properties.usingAOMap        = 0.0f;
			properties.usingEmissiveMap  = 0.0f;

			defaultMaterial = std::make_shared<Material>(shader, properties);
			defaultMaterial->createDescriptorSet();
			defaultMaterial->setRenderFlag(Material::RenderFlags::ForwardPreviewRender);
			defaultMaterial->removeRenderFlag(Material::RenderFlags::DeferredRender);
			defaultMaterial->setAlbedo(Texture2D::getDefaultTexture());
		}
	};

	struct RenderGraph::SkyboxData
	{
		std::shared_ptr<Mesh>          skyboxMesh;
		std::shared_ptr<Shader>        skyboxShader;
		std::shared_ptr<DescriptorSet> skyboxDescriptorSet;
		glm::mat4                      projView;
	};

	struct RenderGraph::SSRData
	{
		bool                           enable = true;
		std::shared_ptr<DescriptorSet> ssrDescriptorSet;
		std::shared_ptr<Shader>        ssrShader;
		SSRData()
		{
			ssrShader        = Shader::create("shaders/SSR.shader");
			ssrDescriptorSet = DescriptorSet::create({0, ssrShader.get()});
		}
	};

	struct RenderGraph::TAAData
	{
		bool                           enable = true;
		std::shared_ptr<DescriptorSet> taaDescriptorSet;
		std::shared_ptr<Shader>        taaShader;
		std::shared_ptr<Shader>        copyShader;
		std::shared_ptr<DescriptorSet> copyDescriptorSet;

		TAAData()
		{
			taaShader        = Shader::create("shaders/TAA.shader");
			taaDescriptorSet = DescriptorSet::create({0, taaShader.get()});

			copyShader        = Shader::create("shaders/Copy.shader");
			copyDescriptorSet = DescriptorSet::create({0, copyShader.get()});
		}
	};

	struct RenderGraph::AtmosphereData
	{
		bool                                        enable = false;
		std::vector<std::shared_ptr<DescriptorSet>> descriptorSets;
		std::shared_ptr<Shader>                     shader;
		std::shared_ptr<Mesh>                       sphere;
		glm::mat4                                   transform = glm::mat4(1);
		AtmosphereData()
		{
			shader = Shader::create("shaders/Atmosphere.shader");
			descriptorSets.resize(2);
			descriptorSets[0] = DescriptorSet::create({0, shader.get()});
			descriptorSets[1] = DescriptorSet::create({1, shader.get()});
			sphere            = Mesh::createSphere();
		}
	};

	RenderGraph::RenderGraph()
	{
		renderers.resize(static_cast<int32_t>(RenderId::Length));
	}

	RenderGraph::~RenderGraph()
	{
		delete shadowData;
		delete forwardData;
		delete previewData;
		delete ssaoData;
		delete ssrData;
	}

	auto RenderGraph::init(uint32_t width, uint32_t height) -> void
	{
		setScreenBufferSize(width, height);
		gBuffer = std::make_shared<GBuffer>(width, height);
		reset();
		shadowData    = new ShadowData();
		forwardData   = new ForwardData();
		deferredData  = new DeferredData();
		previewData   = new PreviewData();
		skyboxData    = new SkyboxData();
		ssaoData      = new SSAOData();
		ssrData       = new SSRData();
		taaData       = new TAAData();
		atmophereData = new AtmosphereData();
		{
			skyboxData->skyboxShader = Shader::create("shaders/Skybox.shader");
			skyboxData->skyboxMesh   = Mesh::createCube();
			DescriptorInfo descriptorInfo{};
			descriptorInfo.layoutIndex      = 0;
			descriptorInfo.shader           = skyboxData->skyboxShader.get();
			skyboxData->skyboxDescriptorSet = DescriptorSet::create(descriptorInfo);
		}

		{
			finalShader = Shader::create("shaders/ScreenPass.shader");
			DescriptorInfo descriptorInfo{};
			descriptorInfo.layoutIndex = 0;
			descriptorInfo.shader      = finalShader.get();
			finalDescriptorSet         = DescriptorSet::create(descriptorInfo);
		}
		{
			stencilShader = Shader::create("shaders/Outline.shader");
			DescriptorInfo descriptorInfo{};
			descriptorInfo.layoutIndex = 0;
			descriptorInfo.shader      = stencilShader.get();
			stencilDescriptorSet       = DescriptorSet::create(descriptorInfo);
		}

		prefilterRenderer = std::make_unique<PrefilterRenderer>();
		prefilterRenderer->init();

		for (auto renderer : renderers)
		{
			if (renderer)
			{
				renderer->init(gBuffer);
			}
		}

		addRender(std::make_shared<Renderer2D>(), RenderId::Render2D);
		addRender(std::make_shared<PostProcessRenderer>(), RenderId::PostProcess);
	}

	auto RenderGraph::beginScene(Scene *scene) -> void
	{
		PROFILE_FUNCTION();
		auto &registry = scene->getRegistry();

		auto camera = scene->getCamera();
		if (camera.first == nullptr || camera.second == nullptr)
		{
			return;
		}

		transform = camera.second;

		forwardData->commandQueue.clear();
		deferredData->commandQueue.clear();

		const auto &proj     = camera.first->getProjectionMatrix();
		const auto &view     = camera.second->getWorldMatrixInverse();
		auto        projView = proj * view;

		auto &settings = scene->getSettings();

		if (settings.render3D)
		{
			auto descriptorSet = settings.deferredRender ? deferredData->descriptorColorSet[0] : forwardData->descriptorSet[0];
			descriptorSet->setUniform("UniformBufferObject", "projView", &projView);
			descriptorSet->setUniform("UniformBufferObject", "view", &view);
			descriptorSet->setUniform("UniformBufferObject", "projViewOld", &camera.first->getProjectionMatrixOld());

			stencilDescriptorSet->setUniform("UniformBufferObject", "projView", &projView);
			const auto nearPlane = camera.first->getNear();
			const auto farPlane  = camera.first->getFar();
			if (ssaoData->enable)
			{
				ssaoData->ssaoSet[0]->setUniform("UBO", "projection", &proj);
				//ssaoData->ssaoSet[0]->setUniform("UBO", "bias", &ssaoData->bias);
			}

			if (ssrData->enable)
			{
				ssrData->ssrDescriptorSet->setUniform("UniformBufferObject", "view", &view);
				ssrData->ssrDescriptorSet->setUniform("UniformBufferObject", "projection", &proj);
			}

			deferredData->descriptorColorSet[2]->setUniform("UBO", "view", &view);
			deferredData->descriptorColorSet[2]->setUniform("UBO", "nearPlane", &nearPlane);
			deferredData->descriptorColorSet[2]->setUniform("UBO", "farPlane", &farPlane);

			atmophereData->descriptorSets[0]->setUniform("UniformBufferObject", "projView", &projView);
		}

		if (settings.renderSkybox || settings.render3D)
		{
			auto envView = registry.view<Environment>();

			if (envView.size() == 0)
			{
				if (forwardData->skybox)
				{
					forwardData->environmentMap = nullptr;
					forwardData->skybox         = nullptr;
					forwardData->irradianceMap  = nullptr;

					DescriptorInfo descriptorDesc{};
					descriptorDesc.layoutIndex      = 0;
					descriptorDesc.shader           = skyboxData->skyboxShader.get();
					skyboxData->skyboxDescriptorSet = DescriptorSet::create(descriptorDesc);
				}
			}
			else
			{
				//Just use first
				const auto &env = envView.get<Environment>(envView.front());

				if (forwardData->skybox != env.getEnvironment())
				{
					forwardData->environmentMap = env.getPrefilteredEnvironment();
					forwardData->irradianceMap  = env.getIrradianceMap();
					forwardData->skybox         = env.getEnvironment();
				}
			}

			auto inverseCamerm   = view;
			inverseCamerm[3]     = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
			skyboxData->projView = proj * inverseCamerm;
			prefilterRenderer->beginScene(scene);
		}

		Light *directionaLight = nullptr;

		LightData lights[32] = {};
		uint32_t  numLights  = 0;

		if (settings.render3D)
		{
			forwardData->frustum = camera.first->getFrustum(view);
			{
				PROFILE_SCOPE("Get Light");
				auto group = registry.group<Light>(entt::get<Transform>);

				for (auto &lightEntity : group)
				{
					const auto &[light, trans] = group.get<Light, Transform>(lightEntity);
					light.lightData.position   = {trans.getWorldPosition(), 1.f};
					light.lightData.direction  = {glm::normalize(trans.getWorldOrientation() * maple::FORWARD), 1.f};

					if (static_cast<LightType>(light.lightData.type) == LightType::DirectionalLight)
						directionaLight = &light;

					if (static_cast<LightType>(light.lightData.type) != LightType::DirectionalLight)
					{
						/*auto inside = forwardData->frustum.isInsideFast(Sphere(light.lightData.position, light.lightData.radius));

						if (inside == Intersection::Outside)
							continue;*/
					}

					lights[numLights] = light.lightData;
					numLights++;
				}
			}

			auto descriptorSet = settings.deferredRender ? deferredData->descriptorLightSet[0] : forwardData->descriptorSet[2];
			descriptorSet->setUniform("UniformBufferLight", "lights", lights, sizeof(LightData) * numLights, false);
			auto cameraPos = glm::vec4{camera.second->getWorldPosition(), 1.f};
			descriptorSet->setUniform("UniformBufferLight", "cameraPosition", &cameraPos);
		}

		if (settings.renderShadow)
		{
			for (uint32_t i = 0; i < shadowData->shadowMapNum; i++)
			{
				shadowData->cascadeCommandQueue[i].clear();
			}

			if (directionaLight)
			{
				updateCascades(scene, directionaLight);

				for (uint32_t i = 0; i < shadowData->shadowMapNum; i++)
				{
					shadowData->cascadeFrustums[i].from(shadowData->shadowProjView[i]);
				}
			}
		}

		glm::mat4 *shadowTransforms = shadowData->shadowProjView;
		glm::vec4 *splitDepth       = shadowData->splitDepth;
		glm::mat4  lightView        = shadowData->lightMatrix;

		float maxShadowDistance = shadowData->maxShadowDistance;
		float shadowMapSize     = shadowData->shadowMapSize;
		float transitionFade    = shadowData->cascadeTransitionFade;
		float shadowFade        = shadowData->shadowFade;

		if (settings.render3D)
		{
			auto descriptorSet = settings.deferredRender ? deferredData->descriptorLightSet[0] : forwardData->descriptorSet[2];

			descriptorSet->setUniform("UniformBufferLight", "viewMatrix", &view);
			descriptorSet->setUniform("UniformBufferLight", "lightView", &lightView);
			descriptorSet->setUniform("UniformBufferLight", "shadowTransform", shadowTransforms);
			descriptorSet->setUniform("UniformBufferLight", "splitDepths", splitDepth);
			descriptorSet->setUniform("UniformBufferLight", "biasMat", &forwardData->biasMatrix);
			descriptorSet->setUniform("UniformBufferLight", "shadowMapSize", &shadowMapSize);
			descriptorSet->setUniform("UniformBufferLight", "shadowFade", &shadowFade);
			descriptorSet->setUniform("UniformBufferLight", "cascadeTransitionFade", &transitionFade);
			descriptorSet->setUniform("UniformBufferLight", "maxShadowDistance", &maxShadowDistance);
			descriptorSet->setUniform("UniformBufferLight", "initialBias", &shadowData->initialBias);

			auto numShadows       = shadowData->shadowMapNum;
			auto cubeMapMipLevels = forwardData->environmentMap ? forwardData->environmentMap->getMipMapLevels() - 1 : 0;
			descriptorSet->setUniform("UniformBufferLight", "lightCount", &numLights);
			descriptorSet->setUniform("UniformBufferLight", "shadowCount", &numShadows);
			descriptorSet->setUniform("UniformBufferLight", "mode", &forwardData->renderMode);
			descriptorSet->setUniform("UniformBufferLight", "cubeMapMipLevels", &cubeMapMipLevels);
			int32_t ssao = ssaoData->enable ? 1 : 0;
			descriptorSet->setUniform("UniformBufferLight", "ssaoEnable", &ssao);

			descriptorSet->setTexture("uPreintegratedFG", forwardData->preintegratedFG);
			descriptorSet->setTexture("uShadowMap", shadowData->shadowTexture);
			descriptorSet->setTexture("uPrefilterMap", forwardData->environmentMap);
			descriptorSet->setTexture("uIrradianceMap", forwardData->irradianceMap);
		}

		auto group = registry.group<MeshRenderer>(entt::get<Transform>);

		PipelineInfo pipelineInfo{};
		pipelineInfo.shader          = settings.deferredRender ? deferredData->deferredColorShader : forwardData->shader;
		pipelineInfo.polygonMode     = PolygonMode::Fill;
		pipelineInfo.blendMode       = BlendMode::SrcAlphaOneMinusSrcAlpha;
		pipelineInfo.clearTargets    = false;
		pipelineInfo.swapChainTarget = false;

		for (auto entity : group)
		{
			const auto &[mesh, trans] = group.get<MeshRenderer, Transform>(entity);

			//if (mesh.isActive())
			{
				const auto &worldTransform = trans.getWorldMatrix();

				auto bb = mesh.getMesh()->getBoundingBox();
				//auto bbCopy = bb->transform(worldTransform);

				if (directionaLight)
				{
					for (uint32_t i = 0; i < shadowData->shadowMapNum; i++)
					{
						//auto inside = shadowData->cascadeFrustums[i].isInsideFast(bbCopy);

						//if (inside != Intersection::OUTSIDE)
						{
							auto &cmd     = shadowData->cascadeCommandQueue[i].emplace_back();
							cmd.mesh      = mesh.getMesh().get();
							cmd.transform = worldTransform;
						}
					}
				}

				{
					//auto inside = forwardData->frustum.isInsideFast(bbCopy);

					//if (inside != Intersection::Outside)
					{
						auto &cmd     = settings.deferredRender ? deferredData->commandQueue.emplace_back() : forwardData->commandQueue.emplace_back();
						cmd.mesh      = mesh.getMesh().get();
						cmd.transform = worldTransform;
						cmd.material  = mesh.getMesh()->getMaterial() ? mesh.getMesh()->getMaterial().get() : settings.deferredRender ? deferredData->defaultMaterial.get() :
                                                                                                                                        forwardData->defaultMaterial.get();
						cmd.material->bind();

						auto depthTest = settings.deferredRender ? deferredData->depthTest : forwardData->depthTest;
						if (settings.deferredRender)
						{
							pipelineInfo.colorTargets[0] = gBuffer->getBuffer(GBufferTextures::COLOR);
							pipelineInfo.colorTargets[1] = gBuffer->getBuffer(GBufferTextures::POSITION);
							pipelineInfo.colorTargets[2] = gBuffer->getBuffer(GBufferTextures::NORMALS);
							pipelineInfo.colorTargets[3] = gBuffer->getBuffer(GBufferTextures::PBR);
							pipelineInfo.colorTargets[4] = gBuffer->getBuffer(GBufferTextures::VIEW_POSITION);
							pipelineInfo.colorTargets[5] = gBuffer->getBuffer(GBufferTextures::VIEW_NORMALS);
							pipelineInfo.colorTargets[6] = gBuffer->getBuffer(GBufferTextures::VELOCITY);
						}
						else
						{
							pipelineInfo.colorTargets[0] = forwardData->renderTexture;
						}

						pipelineInfo.cullMode            = cmd.material->isFlagOf(Material::RenderFlags::TwoSided) ? CullMode::None : CullMode::Back;
						pipelineInfo.transparencyEnabled = cmd.material->isFlagOf(Material::RenderFlags::AlphaBlend);

						if (depthTest && cmd.material->isFlagOf(Material::RenderFlags::DepthTest))
						{
							pipelineInfo.depthTarget = gBuffer->getDepthBuffer();
						}

						if (registry.try_get<StencilComponent>(entity) != nullptr)
						{
							pipelineInfo.shader                     = stencilShader;
							pipelineInfo.stencilTest                = true;
							pipelineInfo.stencilMask                = 0x00;
							pipelineInfo.stencilFunc                = StencilType::Notequal;
							pipelineInfo.stencilFail                = StencilType::Keep;
							pipelineInfo.stencilDepthFail           = StencilType::Keep;
							pipelineInfo.stencilDepthPass           = StencilType::Replace;
							pipelineInfo.depthTest                  = true;
							cmd.stencilPipelineInfo                 = pipelineInfo;
							cmd.stencilPipelineInfo.colorTargets[0] = gBuffer->getBuffer(GBufferTextures::SCREEN);
							cmd.stencilPipelineInfo.colorTargets[1] = nullptr;
							cmd.stencilPipelineInfo.colorTargets[2] = nullptr;
							cmd.stencilPipelineInfo.colorTargets[3] = nullptr;

							pipelineInfo.shader           = settings.deferredRender ? deferredData->deferredColorShader : forwardData->shader;
							pipelineInfo.stencilMask      = 0xFF;
							pipelineInfo.stencilFunc      = StencilType::Always;
							pipelineInfo.stencilFail      = StencilType::Keep;
							pipelineInfo.stencilDepthFail = StencilType::Keep;
							pipelineInfo.stencilDepthPass = StencilType::Replace;
							pipelineInfo.depthTest        = true;
						}
						cmd.pipelineInfo = pipelineInfo;
					}
				}
			}
		}

		for (auto &renderer : renderers)
		{
			if (renderer)
			{
				renderer->beginScene(scene, projView);
			}
		}

		auto atmosphere = registry.group<Atmosphere>(entt::get<Transform>);

		for (auto atmo : atmosphere)
		{
			auto &asphere = registry.get<Atmosphere>(atmo);
			auto &trans   = registry.get<Transform>(atmo);

			asphere.getData().viewPos = camera.second->getWorldPosition();
			//
			atmophereData->descriptorSets[1]->setUniformBufferData("UniformBuffer", &asphere.getData());
			atmophereData->transform = trans.getWorldMatrix();
			break;
		}
		atmophereData->enable = atmosphere.size() > 0;
	}

	auto RenderGraph::beginPreviewScene(Scene *scene) -> void
	{
		if (scene == nullptr || !previewFocused)
		{
			return;
		}

		auto camera = scene->getCamera();
		if (camera.first == nullptr || camera.second == nullptr)
		{
			return;
		}

		previewData->commandQueue.clear();

		const auto &proj     = camera.first->getProjectionMatrix();
		const auto &view     = camera.second->getWorldMatrixInverse();
		auto        projView = proj * view;

		auto descriptorSets = previewData->descriporSets;
		descriptorSets[0]->setUniform("UniformBufferObject", "projView", &projView);
		descriptorSets[0]->update();

		auto &registry = scene->getRegistry();

		auto group = registry.group<Light>(entt::get<Transform>);

		Light *directionaLight = nullptr;

		for (auto &lightEntity : group)
		{
			const auto &[light, trans] = group.get<Light, Transform>(lightEntity);
			light.lightData.position   = {trans.getWorldPosition(), 1.f};
			light.lightData.direction  = {glm::normalize(trans.getWorldOrientation() * maple::FORWARD), 1.f};

			if (static_cast<LightType>(light.lightData.type) == LightType::DirectionalLight)
				directionaLight = &light;
		}

		auto cameraPos = glm::vec4{camera.second->getWorldPosition(), 1.f};

		descriptorSets[2]->setUniform("UniformBufferLight", "light", &directionaLight->lightData);
		descriptorSets[2]->setUniform("UniformBufferLight", "cameraPosition", glm::value_ptr(cameraPos));

		auto meshGroup = registry.group<MeshRenderer>(entt::get<Transform>);

		for (auto entity : meshGroup)
		{
			const auto &[transform, mesh] = meshGroup.get<Transform, MeshRenderer>(entity);

			auto &data     = previewData->commandQueue.emplace_back();
			data.mesh      = mesh.getMesh().get();
			data.transform = transform.getWorldMatrix();
			data.material  = mesh.getMesh()->getMaterial() == nullptr ? previewData->defaultMaterial.get() : mesh.getMesh()->getMaterial().get();

			if (data.material->getShader() == nullptr)
				data.material->setShader(previewData->shader);

			data.material->bind();
		}
	}

	auto RenderGraph::onRender() -> void
	{
		PROFILE_FUNCTION();

		if (!forwardData->renderTexture)
		{
			LOGW("renderTexture is null");
			return;
		}

		const auto &settings      = Application::getCurrentScene()->getSettings();
		auto        swapChain     = Application::getGraphicsContext()->getSwapChain();
		auto        renderTargert = gBuffer->getBuffer(GBufferTextures::SCREEN);

		//forwardData->renderTexture ? forwardData->renderTexture : swapChain->getCurrentImage();

		if ((settings.deferredRender && deferredData->depthTest) || forwardData->depthTest)
		{
			Application::getRenderDevice()->clearRenderTarget(gBuffer->getDepthBuffer(), getCommandBuffer());
		}

		Application::getRenderDevice()->clearRenderTarget(renderTargert, getCommandBuffer());
		Application::getRenderDevice()->clearRenderTarget(gBuffer->getBuffer(GBufferTextures::COLOR), getCommandBuffer(), {0, 0, 0, 0});
		Application::getRenderDevice()->clearRenderTarget(gBuffer->getBuffer(GBufferTextures::POSITION), getCommandBuffer(), {0, 0, 0, 0});
		Application::getRenderDevice()->clearRenderTarget(gBuffer->getBuffer(GBufferTextures::NORMALS), getCommandBuffer(), {0, 0, 0, 0});
		Application::getRenderDevice()->clearRenderTarget(gBuffer->getBuffer(GBufferTextures::PBR), getCommandBuffer(), {0, 0, 0, 0});

		Application::getRenderDevice()->clearRenderTarget(gBuffer->getBuffer(GBufferTextures::VIEW_POSITION), getCommandBuffer(), {0, 0, 0, 0});
		Application::getRenderDevice()->clearRenderTarget(gBuffer->getBuffer(GBufferTextures::VIEW_NORMALS), getCommandBuffer(), {0, 0, 0, 0});
		Application::getRenderDevice()->clearRenderTarget(gBuffer->getBuffer(GBufferTextures::VELOCITY), getCommandBuffer(), {0, 0, 0, 0});

		if (atmophereData->enable)
		{
			executeAtmospherePass();
		}

		if (settings.renderShadow)
			executeShadowPass();

		if (settings.render3D)
		{
			if (settings.deferredRender)
			{
				executeDeferredOffScreenPass();
				if (ssaoData->enable)
				{
					executeSSAOPass();
					executeSSAOBlurPass();
				}
				executeDeferredLightPass();
			}
			else
			{
				executeForwardPass();
			}
		}

		if (auto render = renderers[static_cast<int32_t>(RenderId::Render2D)]; render != nullptr)
		{
			render->renderScene();
		}

		if (auto render = renderers[static_cast<int32_t>(RenderId::Geometry)]; render != nullptr)
		{
			render->renderScene();
		}

		if (settings.renderSkybox)
		{
			prefilterRenderer->renderScene();
			executeSkyboxPass();
		}

		if (auto render = renderers[static_cast<int32_t>(RenderId::PostProcess)]; render != nullptr)
		{
			render->renderScene();
		}

		if (ssrData->enable)
		{
			executeReflectionPass();
		}

		if (auto render = renderers[static_cast<int32_t>(RenderId::GridRender)]; render != nullptr)
		{
			render->renderScene();
		}

		if (taaData->enable)
		{
			//executeTAAPass();
		}

		executeFinalPass();

		transform = nullptr;
	}

	auto RenderGraph::onRenderPreview() -> void
	{
		if (!previewFocused)
		{
			return;
		}
		PROFILE_FUNCTION();
		/*	if (previewData->renderTexture)
		{
			Application::getRenderDevice()->clearRenderTarget(previewData->renderTexture, getCommandBuffer());
			Application::getRenderDevice()->clearRenderTarget(previewData->depthTexture, getCommandBuffer());
		}*/
		executePreviewPasss();

		for (auto &renderer : renderers)
		{
			renderer->renderPreviewScene();
		}
	}

	auto RenderGraph::executePreviewPasss() -> void
	{
		PROFILE_FUNCTION();
		previewData->descriporSets[2]->update();

		auto commandBuffer = getCommandBuffer();

		std::shared_ptr<Pipeline> pipeline;

		PipelineInfo pipelineInfo{};
		pipelineInfo.shader          = previewData->shader;
		pipelineInfo.polygonMode     = PolygonMode::Fill;
		pipelineInfo.blendMode       = BlendMode::SrcAlphaOneMinusSrcAlpha;
		pipelineInfo.clearTargets    = false;
		pipelineInfo.swapChainTarget = false;
		pipelineInfo.colorTargets[0] = previewData->renderTexture;
		pipelineInfo.depthTarget     = previewData->depthTexture;

		for (auto &command : previewData->commandQueue)
		{
			Mesh *mesh = command.mesh;

			const auto &worldTransform = command.transform;

			Material *material = command.material;

			pipelineInfo.cullMode            = command.material->isFlagOf(Material::RenderFlags::TwoSided) ? CullMode::None : CullMode::Back;
			pipelineInfo.transparencyEnabled = command.material->isFlagOf(Material::RenderFlags::AlphaBlend);
			pipeline                         = Pipeline::get(pipelineInfo);

			if (commandBuffer)
				commandBuffer->bindPipeline(pipeline.get());
			else
				pipeline->bind(commandBuffer);

			previewData->descriporSets[1] = material->getDescriptorSet();

			auto &pushConstants = previewData->shader->getPushConstants()[0];
			pushConstants.setValue("transform", (void *) &worldTransform);

			previewData->shader->bindPushConstants(commandBuffer, pipeline.get());

			Renderer::bindDescriptorSets(pipeline.get(), commandBuffer, 0, previewData->descriporSets);
			Renderer::drawMesh(commandBuffer, pipeline.get(), mesh);
		}

		if (commandBuffer)
			commandBuffer->unbindPipeline();

		if (pipeline)        //temp
		{
			pipeline->end(commandBuffer);
		}
	}

	auto RenderGraph::onUpdate(const Timestep &step, Scene *scene) -> void
	{
		PROFILE_FUNCTION();
	}

	auto RenderGraph::addRender(const std::shared_ptr<Renderer> &render, int32_t renderPriority) -> void
	{
		PROFILE_FUNCTION();
		render->init(gBuffer);
		renderers[renderPriority] = render;
	}

	auto RenderGraph::reset() -> void
	{
		PROFILE_FUNCTION();
	}

	auto RenderGraph::onResize(uint32_t width, uint32_t height) -> void
	{
		PROFILE_FUNCTION();
		setScreenBufferSize(width, height);
		gBuffer->resize(width, height);
		for (auto &renderer : renderers)
		{
			if (renderer)
				renderer->onResize(width, height);
		}
	}

	auto RenderGraph::onImGui() -> void
	{
		PROFILE_FUNCTION();

		ImGui::TextUnformatted("Shadow Renderer");

		ImGui::DragFloat("Initial Bias", &shadowData->initialBias, 0.00005f, 0.0f, 1.0f, "%.6f");
		ImGui::DragFloat("Light Size", &shadowData->lightSize, 0.00005f, 0.0f, 10.0f);
		ImGui::DragFloat("Max Shadow Distance", &shadowData->maxShadowDistance, 0.05f, 0.0f, 10000.0f);
		ImGui::DragFloat("Shadow Fade", &shadowData->shadowFade, 0.0005f, 0.0f, 500.0f);
		ImGui::DragFloat("Cascade Transition Fade", &shadowData->cascadeTransitionFade, 0.0005f, 0.0f, 5.0f);
		ImGui::DragFloat("Cascade Split Lambda", &shadowData->cascadeSplitLambda, 0.005f, 0.0f, 3.0f);
		//ImGui::DragFloat("Scene Radius Multiplier", &shadowData->sceneRadiusMultiplier, 0.005f, 0.0f, 5.0f);

		ImGui::Separator();

		ImGui::TextUnformatted("Deferred Renderer");

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
		ImGui::Columns(2);
		ImGui::Separator();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Deferred Queue Size");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		ImGui::Text("%5.2lu", deferredData->commandQueue.size());
		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Render Mode");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if (ImGui::BeginMenu(renderOutputMode(forwardData->renderMode).c_str()))
		{
			constexpr int32_t numRenderModes = 15;

			for (int32_t i = 0; i < numRenderModes; i++)
			{
				if (ImGui::MenuItem(renderOutputMode(i).c_str(), "", forwardData->renderMode == i, true))
				{
					forwardData->renderMode = i;
				}
			}
			ImGui::EndMenu();
		}
		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::Columns(1);

		ImGui::Separator();
		ImGui::TextUnformatted("CubeMap");
		ImGui::DragFloat("CubeMap LodLevel", &forwardData->cubeMapLevel, 0.5f, 0.0f, 4.0f);

		if (ImGui::BeginMenu(cubeMapModeToString(forwardData->cubeMapMode).c_str()))
		{
			constexpr int32_t numModes = 3;

			for (int32_t i = 0; i < numModes; i++)
			{
				if (ImGui::MenuItem(cubeMapModeToString(i).c_str(), "", forwardData->cubeMapMode == i, true))
				{
					forwardData->cubeMapMode = i;
				}
			}
			ImGui::EndMenu();
		}

		ImGui::Separator();
		ImGui::TextUnformatted("SSAO Options");
		ImGui::Checkbox("SSAO Enabled", &ssaoData->enable);
		ImGui::DragFloat("SSAO Depth Bias", &ssaoData->bias, 0.00005f, 0.0f, 1.0f, "%.6f");

		ImGui::Separator();
		ImGui::TextUnformatted("SSR Options");
		ImGui::Checkbox("SSR Enabled", &ssrData->enable);
		ImGui::Separator();

		ImGui::Columns(2);
		for (auto shader : Application::getCache()->getCache())
		{
			if (shader.second->getResourceType() == FileType::Shader)
			{
				ImGui::PushID(shader.second.get());
				if (ImGui::Button("Reload"))
				{
					std::static_pointer_cast<Shader>(shader.second)->reload();
				}
				ImGui::PopID();
				ImGui::NextColumn();
				ImGui::TextUnformatted(shader.second->getPath().c_str());
				ImGui::NextColumn();
				ImGui::Separator();
			}
		}

		ImGui::Columns(1);
		ImGui::Separator();
		/*ImGui::TextUnformatted("2D renderer");
		ImGui::Columns(2);

		ImGuiHelpers::Property("Number of draw calls", (int &) m_Renderer2DData.m_BatchDrawCallIndex, ImGuiHelpers::PropertyFlag::ReadOnly);
		ImGuiHelpers::Property("Max textures Per draw call", (int &) m_Renderer2DData.m_Limits.MaxTextures, 1, 16);
		ImGuiHelpers::Property("ToneMap Index", m_ToneMapIndex);
		ImGuiHelpers::Property("Exposure", m_Exposure);

		ImGui::Columns(1);*/

		ImGui::Separator();
		ImGui::PopStyleVar();
	}

	auto RenderGraph::executeForwardPass() -> void
	{
		PROFILE_FUNCTION();
		GPUProfile("Forward Pass");

		/*
		* forwardData->descriptorSet[0]->update();
		forwardData->descriptorSet[2]->update();

		auto commandBuffer = getCommandBuffer();

		Pipeline *pipeline = nullptr;

		for (auto &command : forwardData->commandQueue)
		{
			Mesh *mesh = command.mesh;

			const auto &worldTransform = command.transform;

			Material *material = command.material;

			pipeline = command.pipeline.get();

			if (commandBuffer)
				commandBuffer->bindPipeline(pipeline);
			else
				pipeline->bind(commandBuffer);

			forwardData->descriptorSet[1] = material->getDescriptorSet();

			auto &pushConstants = forwardData->shader->getPushConstants()[0];
			pushConstants.setValue("transform", (void *) &worldTransform);

			forwardData->shader->bindPushConstants(commandBuffer, pipeline);

			Renderer::bindDescriptorSets(pipeline, commandBuffer, 0, forwardData->descriptorSet);
			Renderer::drawMesh(commandBuffer, pipeline, mesh);
		}

		if (commandBuffer)
			commandBuffer->unbindPipeline();

		if (pipeline)        //temp
		{
			pipeline->end(commandBuffer);
		}*/
	}

	auto RenderGraph::executeShadowPass() -> void
	{
		PROFILE_FUNCTION();

		shadowData->descriptorSet[0]->setUniform("UniformBufferObject", "projView", shadowData->shadowProjView);
		shadowData->descriptorSet[0]->update();

		PipelineInfo pipelineInfo;
		pipelineInfo.shader = shadowData->shader;

		pipelineInfo.cullMode            = CullMode::None;
		pipelineInfo.transparencyEnabled = false;
		pipelineInfo.depthBiasEnabled    = false;
		pipelineInfo.depthArrayTarget    = shadowData->shadowTexture;
		pipelineInfo.clearTargets        = true;

		auto pipeline = Pipeline::get(pipelineInfo);

		for (uint32_t i = 0; i < shadowData->shadowMapNum; ++i)
		{
			//GPUProfile("Shadow Layer Pass");
			pipeline->bind(getCommandBuffer(), i);

			for (auto &command : shadowData->cascadeCommandQueue[i])
			{
				Mesh *mesh                           = command.mesh;
				shadowData->currentDescriptorSets[0] = shadowData->descriptorSet[0];
				auto  trans                          = command.transform;
				auto &pushConstants                  = shadowData->shader->getPushConstants()[0];

				pushConstants.setValue("transform", (void *) &trans);
				pushConstants.setValue("cascadeIndex", (void *) &i);

				shadowData->shader->bindPushConstants(getCommandBuffer(), pipeline.get());

				Renderer::bindDescriptorSets(pipeline.get(), getCommandBuffer(), 0, shadowData->descriptorSet);
				Renderer::drawMesh(getCommandBuffer(), pipeline.get(), mesh);
			}
			pipeline->end(getCommandBuffer());
		}
	}

	auto RenderGraph::executeSkyboxPass() -> void
	{
		PROFILE_FUNCTION();
		GPUProfile("SkyBox Pass");
		if (forwardData->skybox == nullptr)
		{
			return;
		}

		PipelineInfo pipelineInfo{};
		pipelineInfo.shader = skyboxData->skyboxShader;

		pipelineInfo.polygonMode         = PolygonMode::Fill;
		pipelineInfo.cullMode            = CullMode::Front;
		pipelineInfo.transparencyEnabled = false;

		pipelineInfo.depthTarget     = gBuffer->getDepthBuffer();
		pipelineInfo.colorTargets[0] = gBuffer->getBuffer(GBufferTextures::SCREEN);

		skyboxPipeline = Pipeline::get(pipelineInfo);
		skyboxPipeline->bind(getCommandBuffer());
		if (forwardData->cubeMapMode == 0)
		{
			skyboxData->skyboxDescriptorSet->setTexture("uCubeMap", forwardData->skybox);
		}
		else if (forwardData->cubeMapMode == 1)
		{
			skyboxData->skyboxDescriptorSet->setTexture("uCubeMap", forwardData->environmentMap);
		}
		else if (forwardData->cubeMapMode == 2)
		{
			skyboxData->skyboxDescriptorSet->setTexture("uCubeMap", forwardData->irradianceMap);
		}

		skyboxData->skyboxDescriptorSet->setUniform("UniformBufferObjectLod", "lodLevel", &forwardData->cubeMapLevel);
		skyboxData->skyboxDescriptorSet->update();

		auto &constants = skyboxData->skyboxShader->getPushConstants();
		constants[0].setValue("projView", glm::value_ptr(skyboxData->projView));
		skyboxData->skyboxShader->bindPushConstants(getCommandBuffer(), skyboxPipeline.get());

		Renderer::bindDescriptorSets(skyboxPipeline.get(), getCommandBuffer(), 0, {skyboxData->skyboxDescriptorSet});
		Renderer::drawMesh(getCommandBuffer(), skyboxPipeline.get(), skyboxData->skyboxMesh.get());
		skyboxPipeline->end(getCommandBuffer());
	}

	auto RenderGraph::executeAtmospherePass() -> void
	{
		PROFILE_FUNCTION();
		GPUProfile("Atmosphere Pass");
		if (atmophereData->sphere == nullptr)
		{
			return;
		}
		atmophereData->descriptorSets[0]->update();
		atmophereData->descriptorSets[1]->update();

		PipelineInfo pipelineInfo{};
		pipelineInfo.shader = atmophereData->shader;

		pipelineInfo.polygonMode         = PolygonMode::Fill;
		pipelineInfo.cullMode            = CullMode::Front;
		pipelineInfo.transparencyEnabled = false;

		pipelineInfo.depthTarget     = gBuffer->getDepthBuffer();
		pipelineInfo.colorTargets[0] = gBuffer->getBuffer(GBufferTextures::SCREEN);

		auto pipeline = Pipeline::get(pipelineInfo);
		pipeline->bind(getCommandBuffer());

		auto &constants = atmophereData->shader->getPushConstants();
		constants[0].setValue("transform", glm::value_ptr(atmophereData->transform));
		atmophereData->shader->bindPushConstants(getCommandBuffer(), pipeline.get());

		Renderer::bindDescriptorSets(pipeline.get(), getCommandBuffer(), 0, atmophereData->descriptorSets);
		Renderer::drawMesh(getCommandBuffer(), pipeline.get(), atmophereData->sphere.get());
		pipeline->end(getCommandBuffer());
	}

	auto RenderGraph::executeDeferredOffScreenPass() -> void
	{
		PROFILE_FUNCTION();
		deferredData->descriptorColorSet[0]->update();
		deferredData->descriptorColorSet[2]->update();
		stencilDescriptorSet->update();

		auto                      commandBuffer = getCommandBuffer();
		std::shared_ptr<Pipeline> pipeline;

		for (auto &command : deferredData->commandQueue)
		{
			pipeline = Pipeline::get(command.pipelineInfo);

			if (commandBuffer)
				commandBuffer->bindPipeline(pipeline.get());
			else
				pipeline->bind(commandBuffer);

			deferredData->descriptorColorSet[1] = command.material->getDescriptorSet();
			auto &pushConstants                 = deferredData->deferredColorShader->getPushConstants()[0];
			pushConstants.setValue("transform", &command.transform);
			deferredData->deferredColorShader->bindPushConstants(commandBuffer, pipeline.get());
			Renderer::bindDescriptorSets(pipeline.get(), commandBuffer, 0, deferredData->descriptorColorSet);
			Renderer::drawMesh(commandBuffer, pipeline.get(), command.mesh);

			if (command.stencilPipelineInfo.stencilTest)
			{
				auto stencilPipeline = Pipeline::get(command.stencilPipelineInfo);

				if (commandBuffer)
					commandBuffer->bindPipeline(stencilPipeline.get());
				else
					stencilPipeline->bind(commandBuffer);

				auto &pushConstants = stencilShader->getPushConstants()[0];
				pushConstants.setValue("transform", &command.transform);
				stencilShader->bindPushConstants(commandBuffer, stencilPipeline.get());

				Renderer::bindDescriptorSets(stencilPipeline.get(), commandBuffer, 0, {stencilDescriptorSet});
				Renderer::drawMesh(commandBuffer, stencilPipeline.get(), command.mesh);

				if (commandBuffer)
					commandBuffer->unbindPipeline();
				else
					stencilPipeline->end(commandBuffer);
			}
		}

		if (commandBuffer)
			commandBuffer->unbindPipeline();
		else if (pipeline)
			pipeline->end(commandBuffer);
	}

	auto RenderGraph::executeDeferredLightPass() -> void
	{
		PROFILE_FUNCTION();
		auto descriptorSet = deferredData->descriptorLightSet[0];
		descriptorSet->setTexture("uColorSampler", gBuffer->getBuffer(GBufferTextures::COLOR));
		descriptorSet->setTexture("uPositionSampler", gBuffer->getBuffer(GBufferTextures::POSITION));
		descriptorSet->setTexture("uNormalSampler", gBuffer->getBuffer(GBufferTextures::NORMALS));
		descriptorSet->setTexture("uPBRSampler", gBuffer->getBuffer(GBufferTextures::PBR));
		descriptorSet->setTexture("uSSAOSampler", gBuffer->getBuffer(GBufferTextures::SSAO_BLUR));
		descriptorSet->setTexture("uDepthSampler", gBuffer->getDepthBuffer());
		descriptorSet->setTexture("uIrradianceMap", forwardData->irradianceMap);
		descriptorSet->setTexture("uPreintegratedFG", forwardData->preintegratedFG);
		descriptorSet->setTexture("uShadowMap", shadowData->shadowTexture);
		descriptorSet->setTexture("uPrefilterMap", forwardData->environmentMap);
		descriptorSet->setTexture("uSSAOSampler0", gBuffer->getBuffer(GBufferTextures::SSAO_SCREEN));
		descriptorSet->setTexture("uViewPositionSampler", gBuffer->getBuffer(GBufferTextures::VIEW_POSITION));
		descriptorSet->setTexture("uViewNormalSampler", gBuffer->getBuffer(GBufferTextures::VIEW_NORMALS));
		descriptorSet->update();

		PipelineInfo pipeInfo;
		pipeInfo.shader                     = deferredData->deferredLightShader;
		pipeInfo.polygonMode                = PolygonMode::Fill;
		pipeInfo.cullMode                   = CullMode::None;
		pipeInfo.transparencyEnabled        = false;
		pipeInfo.depthBiasEnabled           = false;
		pipeInfo.clearTargets               = false;
		pipeInfo.colorTargets[0]            = gBuffer->getBuffer(GBufferTextures::SCREEN);
		deferredData->deferredLightPipeline = Pipeline::get(pipeInfo);

		deferredData->deferredLightPipeline->bind(getCommandBuffer());
		Renderer::bindDescriptorSets(deferredData->deferredLightPipeline.get(), getCommandBuffer(), 0, deferredData->descriptorLightSet);
		Renderer::drawMesh(getCommandBuffer(), deferredData->deferredLightPipeline.get(), deferredData->screenQuad.get());
		deferredData->deferredLightPipeline->end(getCommandBuffer());
	}

	auto RenderGraph::executeSSAOPass() -> void
	{
		PROFILE_FUNCTION();
		auto descriptorSet = ssaoData->ssaoSet[0];
		descriptorSet->setTexture("uViewPositionSampler", gBuffer->getBuffer(GBufferTextures::VIEW_POSITION));
		descriptorSet->setTexture("uViewNormalSampler", gBuffer->getBuffer(GBufferTextures::VIEW_NORMALS));
		descriptorSet->setTexture("uSsaoNoise", gBuffer->getSSAONoise());
		descriptorSet->update();

		auto commandBuffer = getCommandBuffer();

		PipelineInfo pipeInfo;
		pipeInfo.shader              = ssaoData->ssaoShader;
		pipeInfo.polygonMode         = PolygonMode::Fill;
		pipeInfo.cullMode            = CullMode::None;
		pipeInfo.transparencyEnabled = false;
		pipeInfo.depthBiasEnabled    = false;
		pipeInfo.clearTargets        = true;
		pipeInfo.depthTest           = false;
		pipeInfo.colorTargets[0]     = gBuffer->getBuffer(GBufferTextures::SSAO_SCREEN);
		auto pipeline                = Pipeline::get(pipeInfo);

		if (commandBuffer)
			commandBuffer->bindPipeline(pipeline.get());
		else
			pipeline->bind(getCommandBuffer());

		Renderer::bindDescriptorSets(pipeline.get(), getCommandBuffer(), 0, ssaoData->ssaoSet);
		Renderer::drawMesh(getCommandBuffer(), pipeline.get(), deferredData->screenQuad.get());

		if (commandBuffer)
			commandBuffer->unbindPipeline();
		else
			pipeline->end(getCommandBuffer());
	}

	auto RenderGraph::executeSSAOBlurPass() -> void
	{
		PROFILE_FUNCTION();
		auto descriptorSet = ssaoData->ssaoBlurSet[0];
		descriptorSet->setTexture("uSsaoSampler", gBuffer->getBuffer(GBufferTextures::SSAO_SCREEN));
		descriptorSet->update();

		auto commandBuffer = getCommandBuffer();

		PipelineInfo pipeInfo;
		pipeInfo.shader              = ssaoData->ssaoBlurShader;
		pipeInfo.polygonMode         = PolygonMode::Fill;
		pipeInfo.cullMode            = CullMode::None;
		pipeInfo.transparencyEnabled = false;
		pipeInfo.depthBiasEnabled    = false;
		pipeInfo.clearTargets        = true;
		pipeInfo.depthTest           = false;
		pipeInfo.colorTargets[0]     = gBuffer->getBuffer(GBufferTextures::SSAO_BLUR);
		auto pipeline                = Pipeline::get(pipeInfo);

		if (commandBuffer)
			commandBuffer->bindPipeline(pipeline.get());
		else
			pipeline->bind(getCommandBuffer());

		Renderer::bindDescriptorSets(pipeline.get(), getCommandBuffer(), 0, ssaoData->ssaoBlurSet);
		Renderer::drawMesh(getCommandBuffer(), pipeline.get(), deferredData->screenQuad.get());

		if (commandBuffer)
			commandBuffer->unbindPipeline();
		else
			pipeline->end(getCommandBuffer());
	}

	auto RenderGraph::executeFXAA() -> void
	{
	}

	auto RenderGraph::executeReflectionPass() -> void
	{
		PROFILE_FUNCTION();
		if (gBuffer->getWidth() == 0 || gBuffer->getHeight() == 0)
		{
			return;
		}
		ssrData->ssrDescriptorSet->setTexture("uViewPositionSampler", gBuffer->getBuffer(GBufferTextures::VIEW_POSITION));
		ssrData->ssrDescriptorSet->setTexture("uViewNormalSampler", gBuffer->getBuffer(GBufferTextures::VIEW_NORMALS));
		ssrData->ssrDescriptorSet->setTexture("uPBRSampler", gBuffer->getBuffer(GBufferTextures::PBR));
		ssrData->ssrDescriptorSet->setTexture("uScreenSampler", gBuffer->getBuffer(GBufferTextures::SCREEN));
		ssrData->ssrDescriptorSet->update();

		auto commandBuffer = getCommandBuffer();

		PipelineInfo pipeInfo;
		pipeInfo.shader              = ssrData->ssrShader;
		pipeInfo.polygonMode         = PolygonMode::Fill;
		pipeInfo.cullMode            = CullMode::None;
		pipeInfo.transparencyEnabled = false;
		pipeInfo.depthBiasEnabled    = false;
		pipeInfo.clearTargets        = true;
		pipeInfo.depthTest           = false;
		pipeInfo.colorTargets[0]     = gBuffer->getBuffer(GBufferTextures::SSR_SCREEN);

		//deferredData->renderTexture;

		auto pipeline = Pipeline::get(pipeInfo);

		if (commandBuffer)
			commandBuffer->bindPipeline(pipeline.get());
		else
			pipeline->bind(getCommandBuffer());

		Renderer::bindDescriptorSets(pipeline.get(), getCommandBuffer(), 0, {ssrData->ssrDescriptorSet});
		Renderer::drawMesh(getCommandBuffer(), pipeline.get(), deferredData->screenQuad.get());

		if (commandBuffer)
			commandBuffer->unbindPipeline();
		else
			pipeline->end(commandBuffer);
	}

	auto RenderGraph::executeTAAPass() -> void
	{
		PROFILE_FUNCTION();
		if (gBuffer->getWidth() == 0 || gBuffer->getHeight() == 0)
		{
			return;
		}

		static bool first = true;

		auto commandBuffer = getCommandBuffer();

		taaData->taaDescriptorSet->setTexture("uViewPositionSampler", gBuffer->getBuffer(GBufferTextures::VIEW_POSITION));
		taaData->taaDescriptorSet->setTexture("uNormalVelocity", gBuffer->getBuffer(GBufferTextures::VELOCITY));
		taaData->taaDescriptorSet->setTexture("uScreenSampler", gBuffer->getBuffer(GBufferTextures::SCREEN));
		taaData->taaDescriptorSet->setTexture("uPreviousScreenSampler", gBuffer->getBuffer(GBufferTextures::PREV_DISPLAY));
		taaData->taaDescriptorSet->update();

		PipelineInfo pipeInfo;
		pipeInfo.shader              = taaData->taaShader;
		pipeInfo.polygonMode         = PolygonMode::Fill;
		pipeInfo.cullMode            = CullMode::None;
		pipeInfo.transparencyEnabled = false;
		pipeInfo.depthBiasEnabled    = false;
		pipeInfo.clearTargets        = true;
		pipeInfo.depthTest           = false;
		pipeInfo.colorTargets[0]     = deferredData->renderTexture;

		auto pipeline = Pipeline::get(pipeInfo);

		if (commandBuffer)
			commandBuffer->bindPipeline(pipeline.get());
		else
			pipeline->bind(getCommandBuffer());

		Renderer::bindDescriptorSets(pipeline.get(), getCommandBuffer(), 0, {taaData->taaDescriptorSet});
		Renderer::drawMesh(getCommandBuffer(), pipeline.get(), deferredData->screenQuad.get());

		if (commandBuffer)
			commandBuffer->unbindPipeline();
		else
			pipeline->end(commandBuffer);

		{
			taaData->copyDescriptorSet->setTexture("uScreenSampler", deferredData->renderTexture);
			taaData->copyDescriptorSet->update();
			PipelineInfo pipeInfo;
			pipeInfo.shader              = taaData->copyShader;
			pipeInfo.polygonMode         = PolygonMode::Fill;
			pipeInfo.cullMode            = CullMode::None;
			pipeInfo.transparencyEnabled = false;
			pipeInfo.depthBiasEnabled    = false;
			pipeInfo.clearTargets        = true;
			pipeInfo.depthTest           = false;
			pipeInfo.colorTargets[0]     = gBuffer->getBuffer(GBufferTextures::PREV_DISPLAY);

			auto pipeline = Pipeline::get(pipeInfo);

			if (commandBuffer)
				commandBuffer->bindPipeline(pipeline.get());
			else
				pipeline->bind(getCommandBuffer());

			Renderer::bindDescriptorSets(pipeline.get(), getCommandBuffer(), 0, {taaData->copyDescriptorSet});
			Renderer::drawMesh(getCommandBuffer(), pipeline.get(), deferredData->screenQuad.get());

			if (commandBuffer)
				commandBuffer->unbindPipeline();
			else
				pipeline->end(commandBuffer);
		}
	}

	auto RenderGraph::updateCascades(Scene *scene, Light *light) -> void
	{
		PROFILE_FUNCTION();
		auto camera = scene->getCamera();
		if (camera.first == nullptr || camera.second == nullptr)
			return;

		float cascadeSplits[SHADOWMAP_MAX];

		float nearClip  = camera.first->getNear();
		float farClip   = camera.first->getFar();
		float clipRange = farClip - nearClip;

		float minZ  = nearClip;
		float maxZ  = nearClip + clipRange;
		float range = maxZ - minZ;
		float ratio = maxZ / minZ;
		// Calculate split depths based on view camera frustum
		// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
#pragma omp parallel for
		for (uint32_t i = 0; i < shadowData->shadowMapNum; i++)
		{
			float p          = static_cast<float>(i + 1) / static_cast<float>(shadowData->shadowMapNum);
			float log        = minZ * std::pow(ratio, p);
			float uniform    = minZ + range * p;
			float d          = shadowData->cascadeSplitLambda * (log - uniform) + uniform;
			cascadeSplits[i] = (d - nearClip) / clipRange;
		}

		//cascadeSplits[3] = 0.35f;

		for (uint32_t i = 0; i < shadowData->shadowMapNum; i++)
		{
			PROFILE_SCOPE("Create Cascade");
			float splitDist     = cascadeSplits[i];
			float lastSplitDist = i == 0 ? 0.0f : cascadeSplits[i - 1];

			auto frum = camera.first->getFrustum(glm::inverse(camera.second->getWorldMatrixInverse()));

			glm::vec3 *frustumCorners = frum.vertices;

			for (uint32_t i = 0; i < 4; i++)
			{
				glm::vec3 dist        = frustumCorners[i + 4] - frustumCorners[i];
				frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
				frustumCorners[i]     = frustumCorners[i] + (dist * lastSplitDist);
			}

			glm::vec3 frustumCenter = glm::vec3(0.0f);
			for (uint32_t i = 0; i < 8; i++)
			{
				frustumCenter += frustumCorners[i];
			}
			frustumCenter /= 8.0f;

			float radius = 0.0f;
			for (uint32_t i = 0; i < 8; i++)
			{
				float distance = glm::length(frustumCorners[i] - frustumCenter);
				radius         = glm::max(radius, distance);
			}
			radius = std::ceil(radius * 16.0f) / 16.0f;

			glm::vec3 maxExtents = glm::vec3(radius);
			glm::vec3 minExtents = -maxExtents;

			glm::vec3 lightDir         = glm::normalize(glm::vec3(light->lightData.direction) * -1.f);
			glm::mat4 lightViewMatrix  = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, maple::UP);
			glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);

			shadowData->splitDepth[i]     = glm::vec4(camera.first->getNear() + splitDist * clipRange) * -1.f;
			shadowData->shadowProjView[i] = lightOrthoMatrix * lightViewMatrix;

			if (i == 0)
				shadowData->lightMatrix = lightViewMatrix;
		}
	}

	auto RenderGraph::setRenderTarget(const std::shared_ptr<Texture> &texture, bool rebuildFramebuffer) -> void
	{
		PROFILE_FUNCTION();
		forwardData->renderTexture  = texture;
		deferredData->renderTexture = texture;
		for (auto &renderer : renderers)
		{
			if (renderer)
				renderer->setRenderTarget(texture);
		}
	}

	auto RenderGraph::setPreview(const std::shared_ptr<Texture> &texture, const std::shared_ptr<Texture> &depth) -> void
	{
		previewData->depthTexture  = depth;
		previewData->renderTexture = texture;
	}

	auto RenderGraph::getCommandBuffer() -> CommandBuffer *
	{
		return Application::getGraphicsContext()->getSwapChain()->getCurrentCommandBuffer();
	}

	auto RenderGraph::executeFinalPass() -> void
	{
		PROFILE_FUNCTION();
		//GPUProfile("Final Pass");

		finalDescriptorSet->setUniform("UniformBuffer", "exposure", &exposure);
		finalDescriptorSet->setUniform("UniformBuffer", "toneMapIndex", &toneMapIndex);
		auto ssaoEnable    = ssaoData->enable ? 1 : 0;
		auto reflectEnable = ssrData->enable ? 1 : 0;
		finalDescriptorSet->setUniform("UniformBuffer", "ssaoEnable", &ssaoEnable);
		finalDescriptorSet->setUniform("UniformBuffer", "reflectEnable", &reflectEnable);

		finalDescriptorSet->setTexture("uScreenSampler", gBuffer->getBuffer(GBufferTextures::SCREEN));
		finalDescriptorSet->setTexture("uReflectionSampler", gBuffer->getBuffer(GBufferTextures::SSR_SCREEN));
		finalDescriptorSet->update();

		PipelineInfo pipelineDesc{};
		pipelineDesc.shader = finalShader;

		pipelineDesc.polygonMode         = PolygonMode::Fill;
		pipelineDesc.cullMode            = CullMode::Back;
		pipelineDesc.transparencyEnabled = false;

		if (forwardData->renderTexture)
			pipelineDesc.colorTargets[0] = forwardData->renderTexture;
		else
			pipelineDesc.swapChainTarget = true;

		auto pipeline = Pipeline::get(pipelineDesc);
		pipeline->bind(getCommandBuffer());

		Application::getRenderDevice()->bindDescriptorSets(pipeline.get(), getCommandBuffer(), 0, {finalDescriptorSet});

		Renderer::bindDescriptorSets(pipeline.get(), getCommandBuffer(), 0, {finalDescriptorSet});
		Renderer::drawMesh(getCommandBuffer(), pipeline.get(), deferredData->screenQuad.get());

		pipeline->end(getCommandBuffer());
	}

};        // namespace maple
