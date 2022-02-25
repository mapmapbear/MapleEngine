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
#include "Engine/CaptureGraph.h"

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
#include "GridRenderer.h"
#include "GeometryRenderer.h"
#include "FinalPass.h"

#include "Others/Randomizer.h"
#include "ImGui/ImGuiHelpers.h"

#include <ecs/ecs.h>

namespace maple
{

	namespace on_begin_renderer 
	{
		using Entity = ecs::Chain
			::Read<component::RendererData>
			::To<ecs::Entity>;

		inline auto system(Entity entity, ecs::World world)
		{
			auto [renderer] = entity;
			if (renderer.gbuffer == nullptr)
				return;
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
	}        // namespace

	auto RenderGraph::init(uint32_t width, uint32_t height) -> void
	{
		gBuffer = std::make_shared<GBuffer>(width, height);

		auto executePoint = Application::getExecutePoint();

		executePoint->registerGlobalComponent<component::RendererData>([&](component::RendererData& data) {
			data.screenQuad = Mesh::createQuad(true);
			data.gbuffer = gBuffer.get();
		});

		executePoint->registerGlobalComponent<capture_graph::component::RenderGraph>();

		executePoint->registerGlobalComponent<component::CameraView>();
		executePoint->registerGlobalComponent<component::FinalPass>();


		static ExecuteQueue beginQ;
		static ExecuteQueue renderQ;

		executePoint->registerQueue(beginQ);
		executePoint->registerQueue(renderQ);
		executePoint->registerWithinQueue<on_begin_renderer::system>(renderQ);

		reflective_shadow_map::registerShadowMap(beginQ, renderQ, executePoint);
		deferred_offscreen::registerDeferredOffScreenRenderer(beginQ, renderQ, executePoint);
		post_process::registerSSAOPass(beginQ, renderQ, executePoint);
		deferred_lighting::registerDeferredLighting(beginQ, renderQ, executePoint);
		atmosphere_pass::registerAtmosphere(beginQ,renderQ, executePoint);
		skybox_renderer::registerSkyboxRenderer(beginQ, renderQ, executePoint);
		cloud_renderer::registerCloudRenderer(beginQ, renderQ, executePoint);
		render2d::registerRenderer2D(beginQ,renderQ, executePoint);
		post_process::registerSSR(renderQ, executePoint);
		grid_renderer::registerGridRenderer(beginQ, renderQ, executePoint);
		geometry_renderer::registerGeometryRenderer(beginQ, renderQ, executePoint);
		final_screen_pass::registerFinalPass(renderQ, executePoint);
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
		cameraView.fov = camera.first->getFov();
	}

	auto RenderGraph::beginPreviewScene(Scene *scene) -> void
	{
		if (scene == nullptr || !previewFocused)
		{
			return;
		}

/*
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
		}*/
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
	/*	PROFILE_FUNCTION();
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
		}*/
	}

	auto RenderGraph::onUpdate(const Timestep &step, Scene *scene) -> void
	{
		PROFILE_FUNCTION();
		auto& renderData = scene->getGlobalComponent<component::RendererData>();
		auto& winSize = scene->getGlobalComponent<component::WindowSize>();
		winSize.height = screenBufferHeight;
		winSize.width = screenBufferWidth;
		renderData.commandBuffer = Application::getGraphicsContext()->getSwapChain()->getCurrentCommandBuffer();
	}

	auto RenderGraph::onResize(uint32_t width, uint32_t height) -> void
	{
		PROFILE_FUNCTION();
		setScreenBufferSize(width, height);
		gBuffer->resize(width, height);
	}

	auto RenderGraph::onImGui() -> void
	{
		PROFILE_FUNCTION();
/*

		ImGui::TextUnformatted("Shadow Renderer");

		/ *	ImGui::DragFloat("Initial Bias", &shadowData->initialBias, 0.00005f, 0.0f, 1.0f, "%.6f");
		ImGui::DragFloat("Light Size", &shadowData->lightSize, 0.00005f, 0.0f, 10.0f);
		ImGui::DragFloat("Max Shadow Distance", &shadowData->maxShadowDistance, 0.05f, 0.0f, 10000.0f);
		ImGui::DragFloat("Shadow Fade", &shadowData->shadowFade, 0.0005f, 0.0f, 500.0f);
		ImGui::DragFloat("Cascade Transition Fade", &shadowData->cascadeTransitionFade, 0.0005f, 0.0f, 5.0f);
		ImGui::DragFloat("Cascade Split Lambda", &shadowData->cascadeSplitLambda, 0.005f, 0.0f, 3.0f);* /

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
/ *
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
		}* /
		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::Columns(1);

		ImGui::Separator();
		ImGui::TextUnformatted("SSAO Options");
		ImGui::Separator();

		ImGui::Columns(2);
	/ *	ImGuiHelper::property("SSAO Enabled", ssaoData->enable);
		ImGuiHelper::property("SSAO Depth Bias", ssaoData->bias, 0.0f, 1.0f, ImGuiHelper::PropertyFlag::None);* /
		ImGui::Columns(1);

		ImGui::Separator();
		ImGui::TextUnformatted("SSR Options");
		ImGui::Separator();

		ImGui::Columns(2);
	/ *	ImGuiHelper::property("SSR Enabled", ssrData->enable);* /
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
		ImGui::PopStyleVar();*/

	
	}

	auto RenderGraph::setRenderTarget(Scene* scene, const std::shared_ptr<Texture> &texture, bool rebuildFramebuffer) -> void
	{
		PROFILE_FUNCTION();
		scene->getGlobalComponent<component::FinalPass>().renderTarget = texture;
	}
};        // namespace maple
