//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Engine/Core.h"
#include "Engine/Timestep.h"
#include "Renderer.h"
#include <memory>
#include <vector>

namespace maple
{
	class Camera;
	class Scene;
	class GBuffer;
	class Texture;
	class Light;
	class PrefilterRenderer;
	class Transform;

	class MAPLE_EXPORT RenderGraph
	{
	  public:
		RenderGraph();
		~RenderGraph();
		auto init(uint32_t width, uint32_t height) -> void;
		auto addRender(const std::shared_ptr<Renderer> &render, int32_t renderId) -> void;

		inline auto addRender(const std::shared_ptr<Renderer> &render, RenderId renderId)
		{
			addRender(render, static_cast<int32_t>(renderId));
		}

		auto reset() -> void;
		auto onResize(uint32_t width, uint32_t height) -> void;
		auto beginScene(Scene *scene) -> void;

		auto beginPreviewScene(Scene *scene) -> void;

		auto onRender() -> void;
		auto onRenderPreview() -> void;

		auto onUpdate(const Timestep &timeStep, Scene *scene) -> void;
		auto onImGui() -> void;
		auto executeForwardPass() -> void;
		auto executeShadowPass() -> void;

		auto executeDeferredOffScreenPass() -> void;
		auto executeDeferredLightPass() -> void;
		auto executeSSAOPass() -> void;
		auto executeSSAOBlurPass() -> void;

		auto executeFXAA() -> void;

		auto executeReflectionPass() -> void;
		auto executeTAAPass() -> void;

		auto executePreviewPasss() -> void;

		auto setRenderTarget(const std::shared_ptr<Texture> &texture, bool rebuildFramebuffer = true) -> void;

		auto setPreview(const std::shared_ptr<Texture> &texture, const std::shared_ptr<Texture> &depth) -> void;

		inline auto setPreviewFocused(bool previewFocused)
		{
			this->previewFocused = previewFocused;
		}

		inline auto getGBuffer() const
		{
			return gBuffer;
		}

		inline auto setReflectSkyBox(bool reflect)
		{
			reflectSkyBox = reflect;
		}

		inline auto setShadowMap(bool shadow)
		{
			useShadowMap = shadow;
		}

		inline auto getRender(int32_t renderId)
		{
			return renderers[renderId];
		}

		inline auto setScreenBufferSize(uint32_t width, uint32_t height)
		{
			if (width == 0)
				width = 1;
			if (height == 0)
				height = 1;
			screenBufferWidth  = width;
			screenBufferHeight = height;
		}

	  private:
		auto executeFinalPass() -> void;
		struct Config2D;
		struct ShadowData;
		struct ForwardData;
		struct Renderer2DData;
		struct DeferredData;
		struct PreviewData;
		struct SSAOData;
		struct SSRData;
		struct TAAData;
		struct EnvironmentData;

		auto getCommandBuffer() -> CommandBuffer *;

		std::vector<std::shared_ptr<Renderer>> renderers;

		bool previewFocused = false;
		bool reflectSkyBox  = false;
		bool useShadowMap   = false;

		std::shared_ptr<Texture> screenTexture;
		std::shared_ptr<GBuffer> gBuffer;

		uint32_t screenBufferWidth  = 0;
		uint32_t screenBufferHeight = 0;

		ShadowData *          shadowData   = nullptr;
		ForwardData *         forwardData  = nullptr;
		DeferredData *        deferredData = nullptr;
		PreviewData *         previewData  = nullptr;
		SSAOData *            ssaoData     = nullptr;
		SSRData *             ssrData      = nullptr;
		TAAData *             taaData      = nullptr;
		EnvironmentData *     envData      = nullptr;

		std::shared_ptr<Shader>        finalShader;
		std::shared_ptr<DescriptorSet> finalDescriptorSet;

		std::shared_ptr<DescriptorSet> stencilDescriptorSet;
		std::shared_ptr<Shader>        stencilShader;

		std::shared_ptr<Pipeline> skyboxPipeline;

		int32_t toneMapIndex = 7;
		float   gamma        = 2.2f;

		Transform *transform = nullptr;
	};
};        // namespace maple
