//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "RenderGraph.h"
#include "Application.h"
#include "Engine/Camera.h"
#include "Engine/CaptureGraph.h"
#include "Engine/GBuffer.h"
#include "Engine/LPVGI/LPVIndirectLighting.h"
#include "Engine/LPVGI/LightPropagationVolume.h"
#include "Engine/LPVGI/ReflectiveShadowMap.h"
#include "Engine/Material.h"
#include "Engine/Mesh.h"
#include "Engine/PathTracer/PathIntegrator.h"
#include "Engine/Profiler.h"
#include "Engine/Quad2D.h"
#include "Engine/VXGI/DrawVoxel.h"
#include "Engine/VXGI/Voxelization.h"
#include "Engine/Vertex.h"
#include "Engine/Raytrace/AccelerationStructure.h"
#include "Engine/Raytrace/RaytracedShadow.h"

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

#include "FinalPass.h"
#include "GeometryRenderer.h"
#include "GridRenderer.h"
#include "PostProcessRenderer.h"
#include "Renderer2D.h"
#include "RendererData.h"
#include "ShadowRenderer.h"
#include "SkyboxRenderer.h"

#include "ImGui/ImGuiHelpers.h"
#include "Others/Randomizer.h"

#include <ecs/ecs.h>

namespace maple
{
	namespace on_begin_renderer
	{
		using Entity = ecs::Registry ::Modify<component::RendererData>::To<ecs::Entity>;

		inline auto system(Entity entity, ecs::World world)
		{
			auto [renderer] = entity;
			if (renderer.gbuffer == nullptr)
				return;
			auto renderTargert = renderer.gbuffer->getBuffer(GBufferTextures::SCREEN);

			Application::getRenderDevice()->clearRenderTarget(renderer.gbuffer->getDepthBuffer(), renderer.commandBuffer, {0, 0, 0, 0});
			Application::getRenderDevice()->clearRenderTarget(renderTargert, renderer.commandBuffer);

			Application::getRenderDevice()->clearRenderTarget(renderer.gbuffer->getBuffer(GBufferTextures::COLOR), renderer.commandBuffer, {0, 0, 0, 0});
			Application::getRenderDevice()->clearRenderTarget(renderer.gbuffer->getBuffer(GBufferTextures::POSITION), renderer.commandBuffer, {0, 0, 0, 0});
			Application::getRenderDevice()->clearRenderTarget(renderer.gbuffer->getBuffer(GBufferTextures::NORMALS), renderer.commandBuffer, {0, 0, 0, 0});
			Application::getRenderDevice()->clearRenderTarget(renderer.gbuffer->getBuffer(GBufferTextures::PBR), renderer.commandBuffer, {0, 0, 0, 0});

			Application::getRenderDevice()->clearRenderTarget(renderer.gbuffer->getBuffer(GBufferTextures::VIEW_POSITION), renderer.commandBuffer, {0, 0, 0, 0});
			Application::getRenderDevice()->clearRenderTarget(renderer.gbuffer->getBuffer(GBufferTextures::VIEW_NORMALS), renderer.commandBuffer, {0, 0, 0, 0});
			Application::getRenderDevice()->clearRenderTarget(renderer.gbuffer->getBuffer(GBufferTextures::VELOCITY), renderer.commandBuffer, {0, 0, 0, 0});

			renderer.numFrames++;
		}
	}        // namespace on_begin_renderer

	auto RenderGraph::init(uint32_t width, uint32_t height) -> void
	{
		auto executePoint = Application::getExecutePoint();
		gBuffer           = std::make_shared<GBuffer>(width, height);
		executePoint->registerGlobalComponent<component::RendererData>([&, width, height](component::RendererData &data) {
			data.screenQuad = Mesh::createQuad(true);
			data.unitCube   = TextureCube::create(1);
			data.gbuffer    = gBuffer.get();
		});
		executePoint->registerGlobalComponent<capture_graph::component::RenderGraph>();
		executePoint->registerGlobalComponent<component::CameraView>();
		executePoint->registerGlobalComponent<component::FinalPass>();

		static ExecuteQueue beginQ("BegineScene");
		static ExecuteQueue renderQ("OnRender");

		executePoint->registerQueue(beginQ);
		executePoint->registerQueue(renderQ);
		executePoint->registerWithinQueue<on_begin_renderer::system>(renderQ);

		raytracing::registerAccelerationStructureModule(beginQ,executePoint);

		shadow_map::registerShadowMap(beginQ, renderQ, executePoint);
		reflective_shadow_map::registerShadowMap(beginQ, renderQ, executePoint);
		deferred_offscreen::registerDeferredOffScreenRenderer(beginQ, renderQ, executePoint);
		post_process::registerSSAOPass(beginQ, renderQ, executePoint);
		vxgi::registerVXGIIndirectLighting(renderQ, executePoint);
		deferred_lighting::registerDeferredLighting(beginQ, renderQ, executePoint);
		atmosphere_pass::registerAtmosphere(beginQ, renderQ, executePoint);
		skybox_renderer::registerSkyboxRenderer(beginQ, renderQ, executePoint);
		cloud_renderer::registerCloudRenderer(beginQ, renderQ, executePoint);
		render2d::registerRenderer2D(beginQ, renderQ, executePoint);
		post_process::registerSSR(renderQ, executePoint);
		grid_renderer::registerGridRenderer(beginQ, renderQ, executePoint);
		geometry_renderer::registerGeometryRenderer(beginQ, renderQ, executePoint);
		post_process::registerBloom(renderQ, executePoint);
		vxgi_debug::registerVXGIVisualization(beginQ, renderQ, executePoint);
		vxgi::registerVoxelizer(beginQ, renderQ, executePoint);
		final_screen_pass::registerFinalPass(renderQ, executePoint);

		//############################################################################
		
		raytraced_shadow::registerRaytracedShadow(beginQ, renderQ, executePoint);
	
		cloud_renderer::registerComputeCloud(renderQ, executePoint);
		vxgi::registerUpdateRadiace(renderQ, executePoint);
		light_propagation_volume::registerLPV(beginQ, renderQ, executePoint);
		lpv_indirect_lighting::registerLPVIndirectLight(renderQ, executePoint);
		light_propagation_volume::registerLPVDebug(beginQ, renderQ, executePoint);

		path_integrator::registerPathIntegrator(beginQ, renderQ, executePoint);
	}

	auto RenderGraph::beginScene(Scene *scene) -> void
	{
		PROFILE_FUNCTION();

		auto camera = scene->getCamera();
		if (camera.first == nullptr || camera.second == nullptr)
		{
			return;
		}

		auto &cameraView           = Application::getExecutePoint()->getGlobalComponent<component::CameraView>();
		cameraView.proj            = camera.first->getProjectionMatrix();
		cameraView.view            = camera.second->getWorldMatrixInverse();
		cameraView.projView        = cameraView.proj * cameraView.view;
		cameraView.nearPlane       = camera.first->getNear();
		cameraView.farPlane        = camera.first->getFar();
		cameraView.frustum         = camera.first->getFrustum(cameraView.view);
		cameraView.cameraTransform = camera.second;
		cameraView.fov             = camera.first->getFov();
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
		auto &renderData         = Application::getExecutePoint()->getGlobalComponent<component::RendererData>();
		auto &winSize            = Application::getExecutePoint()->getGlobalComponent<component::WindowSize>();
		winSize.height           = screenBufferHeight;
		winSize.width            = screenBufferWidth;
		renderData.commandBuffer = Application::getGraphicsContext()->getSwapChain()->getCurrentCommandBuffer();
		//renderData.commandBuffer = Application::getGraphicsContext()->getSwapChain()->getComputeCmdBuffer();
		renderData.renderDevice = Application::getRenderDevice().get();
	}

	auto RenderGraph::onResize(uint32_t width, uint32_t height) -> void
	{
		PROFILE_FUNCTION();
		setScreenBufferSize(width, height);
		auto cmd = Application::getGraphicsContext()->getSwapChain()->getCurrentCommandBuffer();
		if (cmd != nullptr)
		{
			cmd->addTask([&, width, height](const CommandBuffer *command) {
				gBuffer->resize(width, height, command);
			});
		}
		else
		{
			gBuffer->resize(width, height, cmd);
		}
	}

	auto RenderGraph::onImGui() -> void
	{
		PROFILE_FUNCTION();
	}

	auto RenderGraph::setRenderTarget(Scene *scene, const std::shared_ptr<Texture> &texture, bool rebuildFramebuffer) -> void
	{
		PROFILE_FUNCTION();
		Application::getExecutePoint()->getGlobalComponent<component::FinalPass>().renderTarget = texture;
	}
};        // namespace maple
