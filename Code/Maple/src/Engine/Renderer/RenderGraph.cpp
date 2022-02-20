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
#include "Scene/Component/VolumetricCloud.h"
#include "Scene/Scene.h"

#include "Math/BoundingBox.h"
#include "Math/MathUtils.h"

#include "Window/NativeWindow.h"

#include "AtmosphereRenderer.h"
#include "CloudRenderer.h"
#include "DeferredOffScreenRenderer.h"
#include "Engine/Vientiane/ReflectiveShadowMap.h"
#include "PostProcessRenderer.h"
#include "Renderer2D.h"
#include "RendererData.h"
#include "SkyboxRenderer.h"

#include "Others/Randomizer.h"

#include <chrono>

#include "ImGui/ImGuiHelpers.h"

#include <ecs/ecs.h>

namespace maple
{

	namespace component
	{
		struct FinalPass
		{
			std::shared_ptr<Shader>        finalShader;
			std::shared_ptr<DescriptorSet> finalDescriptorSet;

			std::shared_ptr<Texture> renderTarget;

			FinalPass()
			{
				finalShader = Shader::create("shaders/ScreenPass.shader");
				DescriptorInfo descriptorInfo{};
				descriptorInfo.layoutIndex = 0;
				descriptorInfo.shader = finalShader.get();
				finalDescriptorSet = DescriptorSet::create(descriptorInfo);
			}
		};
	}

	namespace on_begin_renderer 
	{
		using Entity = ecs::Chain
			::Read<component::RendererData>
			::To<ecs::Entity>;

		inline auto system(Entity entity, ecs::World world)
		{
			auto [renderer] = entity;
			auto        swapChain = Application::getGraphicsContext()->getSwapChain();
			auto        renderTargert = renderer.gbuffer->getBuffer(GBufferTextures::SCREEN);

			Application::getRenderDevice()->clearRenderTarget(renderer.gbuffer->getDepthBuffer(), renderer.commandBuffer);
			Application::getRenderDevice()->clearRenderTarget(renderTargert, renderer.commandBuffer);

			Application::getRenderDevice()->clearRenderTarget(renderer.gbuffer->getBuffer(GBufferTextures::COLOR), renderer.commandBuffer, { 0, 0, 0, 0 });
			Application::getRenderDevice()->clearRenderTarget(renderer.gbuffer->getBuffer(GBufferTextures::POSITION), renderer.commandBuffer, { 0, 0, 0, 0 });
			Application::getRenderDevice()->clearRenderTarget(renderer.gbuffer->getBuffer(GBufferTextures::NORMALS), renderer.commandBuffer, { 0, 0, 0, 0 });
			Application::getRenderDevice()->clearRenderTarget(renderer.gbuffer->getBuffer(GBufferTextures::PBR), renderer.commandBuffer, { 0, 0, 0, 0 });

			Application::getRenderDevice()->clearRenderTarget(renderer.gbuffer->getBuffer(GBufferTextures::VIEW_POSITION), renderer.commandBuffer, { 0, 0, 0, 0 });
			Application::getRenderDevice()->clearRenderTarget(renderer.gbuffer->getBuffer(GBufferTextures::VIEW_NORMALS), renderer.commandBuffer, { 0, 0, 0, 0 });
			Application::getRenderDevice()->clearRenderTarget(renderer.gbuffer->getBuffer(GBufferTextures::VELOCITY), renderer.commandBuffer, { 0, 0, 0, 0 });
		}
	}


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
				case 14:
					return "PseudoSky";
				case 15:
					return "FluxSampler";
				default:
					return "Lighting";
			}
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

	struct RenderGraph::ForwardData
	{
		std::shared_ptr<Texture> defaultTexture;

		std::shared_ptr<Texture>   renderTexture;
		std::shared_ptr<Texture>   depthTexture;
		std::shared_ptr<Material>  defaultMaterial;
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

		ForwardData()
		{
			shader = Shader::create("shaders/ForwardPBR.shader");
			commandQueue.reserve(1000);
	
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
			/*
			taaShader        = Shader::create("shaders/TAA.shader");
			taaDescriptorSet = DescriptorSet::create({0, taaShader.get()});

			copyShader        = Shader::create("shaders/Copy.shader");
			copyDescriptorSet = DescriptorSet::create({0, copyShader.get()});*/
		}
	};

	RenderGraph::RenderGraph()
	{
		renderers.resize(static_cast<int32_t>(RenderId::Length));
	}

	RenderGraph::~RenderGraph()
	{
		
	}


	namespace final_screen_pass
	{
		using Entity = ecs::Chain
			::Read<component::FinalPass>
			::Read<component::RendererData>
			::To<ecs::Entity>;

		auto onRender(Entity entity,ecs::World world)
		{
			auto [finalData,renderData] = entity;
			float gamma = 2.2;
			int32_t toneMapIndex = 7;
				
			finalData.finalDescriptorSet->setUniform("UniformBuffer", "gamma", &gamma);
			finalData.finalDescriptorSet->setUniform("UniformBuffer", "toneMapIndex", &toneMapIndex);
			auto ssaoEnable =  0;
			auto reflectEnable =  0;
			auto cloudEnable = false;// envData->cloud ? 1 : 0;

			finalData.finalDescriptorSet->setUniform("UniformBuffer", "ssaoEnable", &ssaoEnable);
			finalData.finalDescriptorSet->setUniform("UniformBuffer", "reflectEnable", &reflectEnable);
			finalData.finalDescriptorSet->setUniform("UniformBuffer", "cloudEnable", &cloudEnable);

			finalData.finalDescriptorSet->setTexture("uScreenSampler", renderData.gbuffer->getBuffer(GBufferTextures::SCREEN));
			finalData.finalDescriptorSet->setTexture("uReflectionSampler", renderData.gbuffer->getBuffer(GBufferTextures::SSR_SCREEN));

			finalData.finalDescriptorSet->update();

			PipelineInfo pipelineDesc{};
			pipelineDesc.shader = finalData.finalShader;

			pipelineDesc.polygonMode = PolygonMode::Fill;
			pipelineDesc.cullMode = CullMode::Back;
			pipelineDesc.transparencyEnabled = false;

			if (finalData.renderTarget)
				pipelineDesc.colorTargets[0] = finalData.renderTarget;
			else
				pipelineDesc.swapChainTarget = true;

			auto pipeline = Pipeline::get(pipelineDesc);
			pipeline->bind(renderData.commandBuffer);

			Application::getRenderDevice()->bindDescriptorSets(pipeline.get(), renderData.commandBuffer, 0, { finalData.finalDescriptorSet });

			Renderer::bindDescriptorSets(pipeline.get(), renderData.commandBuffer, 0, { finalData.finalDescriptorSet });
			Renderer::drawMesh(renderData.commandBuffer, pipeline.get(), renderData.screenQuad.get());

			pipeline->end(renderData.commandBuffer);
		}
	}


	auto RenderGraph::init(uint32_t width, uint32_t height) -> void
	{
		screenQuad = Mesh::createQuad(true);

		setScreenBufferSize(width, height);
		gBuffer = std::make_shared<GBuffer>(width, height);
		reset();
	/*	forwardData  = new ForwardData();
		previewData  = new PreviewData();
		ssaoData     = new SSAOData();
		ssrData      = new SSRData();
		taaData      = new TAAData();*/
		{
		
		}

		for (auto renderer : renderers)
		{
			if (renderer)
			{
				renderer->init(gBuffer);
			}
		}

		addRender(std::make_shared<Renderer2D>(), RenderId::Render2D);
		addRender(std::make_shared<CloudRenderer>(), RenderId::Cloud);
		//addRender(std::make_shared<SkyboxRenderer>(), RenderId::Skybox);
		addRender(std::make_shared<PostProcessRenderer>(), RenderId::PostProcess);
		addRender(std::make_shared<AtmosphereRenderer>(), RenderId::Atmosphere);

		Application::getExecutePoint()->registerGlobalComponent<component::RendererData>([&](component::RendererData& data) {
			data.screenQuad = Mesh::createQuad(true);
			data.gbuffer = gBuffer.get();
		});

		Application::getExecutePoint()->registerGlobalComponent<component::CameraView>();
		Application::getExecutePoint()->registerGlobalComponent<component::FinalPass>();


		static ExecuteQueue beginQ;
		static ExecuteQueue renderQ;

		Application::getExecutePoint()->registerQueue(beginQ);
		Application::getExecutePoint()->registerQueue(renderQ);
		

		Application::getExecutePoint()->registerWithinQueue<on_begin_renderer::system>(renderQ);

		reflective_shadow_map::registerShadowMap(beginQ, renderQ, Application::getExecutePoint());
		deferred_offscreen::registerDeferredOffScreenRenderer(beginQ, renderQ, Application::getExecutePoint());
		deferred_lighting::registerDeferredLighting(beginQ, renderQ, Application::getExecutePoint());
		skybox_renderer::registerSkyboxRenderer(beginQ, renderQ, Application::getExecutePoint());

		Application::getExecutePoint()->registerWithinQueue<final_screen_pass::onRender>(renderQ);
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

		auto& cameraView = scene->getGlobalComponent<component::CameraView>();
		cameraView.proj = camera.first->getProjectionMatrix();
		cameraView.view = camera.second->getWorldMatrixInverse();
		cameraView.projView = cameraView.proj * cameraView.view;
		cameraView.nearPlane = camera.first->getNear();
		cameraView.farPlane = camera.first->getFar();
		cameraView.frustum = camera.first->getFrustum(cameraView.view);
		cameraView.cameraTransform = camera.second;
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

	auto RenderGraph::onRender(Scene *scene) -> void
	{
		PROFILE_FUNCTION();

		if (!forwardData->renderTexture)
		{
			LOGW("renderTexture is null");
			return;
		}

		const auto &settings      = Application::getCurrentScene()->getSettings();


		/*if (settings.renderShadow)
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
			render->renderScene(scene);
		}

		if (auto render = renderers[static_cast<int32_t>(RenderId::Geometry)]; render != nullptr)
		{
			render->renderScene(scene);
		}

		if (auto render = renderers[static_cast<int32_t>(RenderId::Atmosphere)]; render != nullptr)
		{
			render->renderScene(scene);
		}

		if (auto render = renderers[static_cast<int32_t>(RenderId::Skybox)]; render != nullptr)
		{
			render->renderScene(scene);
		}

		if (auto render = renderers[static_cast<int32_t>(RenderId::Cloud)]; render != nullptr)
		{
			render->renderScene(scene);
		}

		if (ssrData->enable)
		{
			executeReflectionPass();
		}

		if (auto render = renderers[static_cast<int32_t>(RenderId::GridRender)]; render != nullptr)
		{
			render->renderScene(scene);
		}

		if (auto render = renderers[static_cast<int32_t>(RenderId::PostProcess)]; render != nullptr)
		{
			render->renderScene(scene);
		}

		executeFinalPass();

		transform = nullptr;*/
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

		/*for (auto &renderer : renderers)
		{
			renderer->renderPreviewScene();
		}*/
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
		auto& renderData = scene->getGlobalComponent<component::RendererData>();
		renderData.commandBuffer = getCommandBuffer();
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

		/*	ImGui::DragFloat("Initial Bias", &shadowData->initialBias, 0.00005f, 0.0f, 1.0f, "%.6f");
		ImGui::DragFloat("Light Size", &shadowData->lightSize, 0.00005f, 0.0f, 10.0f);
		ImGui::DragFloat("Max Shadow Distance", &shadowData->maxShadowDistance, 0.05f, 0.0f, 10000.0f);
		ImGui::DragFloat("Shadow Fade", &shadowData->shadowFade, 0.0005f, 0.0f, 500.0f);
		ImGui::DragFloat("Cascade Transition Fade", &shadowData->cascadeTransitionFade, 0.0005f, 0.0f, 5.0f);
		ImGui::DragFloat("Cascade Split Lambda", &shadowData->cascadeSplitLambda, 0.005f, 0.0f, 3.0f);*/

		ImGui::Separator();

		ImGui::TextUnformatted("Deferred Renderer");

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
		ImGui::Columns(2);
		ImGui::Separator();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Deferred Queue Size");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		//ImGui::Text("%5.2lu", deferredData->commandQueue.size());
		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Render Mode");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
/*
		if (ImGui::BeginMenu(renderOutputMode(forwardData->renderMode).c_str()))
		{
			constexpr int32_t numRenderModes = 16;

			for (int32_t i = 0; i < numRenderModes; i++)
			{
				if (ImGui::MenuItem(renderOutputMode(i).c_str(), "", forwardData->renderMode == i, true))
				{
					forwardData->renderMode = i;
					/ *auto descriptorSet      = deferredData->descriptorLightSet[0];
					switch (i)
					{
						case 11:
							descriptorSet->setTexture("uOutputSampler", gBuffer->getBuffer(GBufferTextures::SSAO_SCREEN));
							break;
						case 12:
							descriptorSet->setTexture("uOutputSampler", gBuffer->getBuffer(GBufferTextures::VIEW_POSITION));
							break;
						case 13:
							descriptorSet->setTexture("uOutputSampler", gBuffer->getBuffer(GBufferTextures::VIEW_NORMALS));
							break;
						case 14:
							descriptorSet->setTexture("uOutputSampler", gBuffer->getBuffer(GBufferTextures::PSEUDO_SKY));
							break;
					}* /
				}
			}
			ImGui::EndMenu();
		}*/
		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::Columns(1);

		ImGui::Separator();
		ImGui::TextUnformatted("SSAO Options");
		ImGui::Separator();

		ImGui::Columns(2);
	/*	ImGuiHelper::property("SSAO Enabled", ssaoData->enable);
		ImGuiHelper::property("SSAO Depth Bias", ssaoData->bias, 0.0f, 1.0f, ImGuiHelper::PropertyFlag::None);*/
		ImGui::Columns(1);

		ImGui::Separator();
		ImGui::TextUnformatted("SSR Options");
		ImGui::Separator();

		ImGui::Columns(2);
	/*	ImGuiHelper::property("SSR Enabled", ssrData->enable);*/
		ImGui::Columns(1);

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
			}
		}

		ImGui::Columns(1);
		ImGui::Separator();

		ImGui::TextUnformatted("Final Pass");

		ImGui::Columns(2);
		ImGuiHelper::property("ToneMap Index", toneMapIndex, 0, 8);
		ImGuiHelper::property("Gamma", gamma, 1.0, 10.0);
		ImGui::Columns(1);

		ImGui::Separator();
		ImGui::PopStyleVar();

		/*for (auto render : renderers)
		{
			render->onImGui();
		}*/
	}

	auto RenderGraph::executeForwardPass() -> void
	{


	}

	auto RenderGraph::executeShadowPass() -> void
	{
		PROFILE_FUNCTION();
	}

	auto RenderGraph::executeDeferredOffScreenPass() -> void
	{
	
	}

	auto RenderGraph::executeDeferredLightPass() -> void
	{
		
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
		Renderer::drawMesh(getCommandBuffer(), pipeline.get(), screenQuad.get());

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
		Renderer::drawMesh(getCommandBuffer(), pipeline.get(), screenQuad.get());

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
		Renderer::drawMesh(getCommandBuffer(), pipeline.get(), screenQuad.get());

		if (commandBuffer)
			commandBuffer->unbindPipeline();
		else
			pipeline->end(commandBuffer);
	}

	auto RenderGraph::executeTAAPass() -> void
	{
		PROFILE_FUNCTION();
		
	}

	auto RenderGraph::setRenderTarget(Scene* scene, const std::shared_ptr<Texture> &texture, bool rebuildFramebuffer) -> void
	{
		PROFILE_FUNCTION();
		scene->getGlobalComponent<component::FinalPass>().renderTarget = texture;
	}

	auto RenderGraph::setPreview(const std::shared_ptr<Texture> &texture, const std::shared_ptr<Texture> &depth) -> void
	{
		/*previewData->depthTexture  = depth;
		previewData->renderTexture = texture;*/
	}

	auto RenderGraph::getCommandBuffer() -> CommandBuffer *
	{
		return Application::getGraphicsContext()->getSwapChain()->getCurrentCommandBuffer();
	}

	auto RenderGraph::executeFinalPass() -> void
	{
		PROFILE_FUNCTION();
		//GPUProfile("Final Pass");


	}

};        // namespace maple
