//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include "RHI/Definitions.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace maple
{
	class Scene;
	class GBuffer;

	enum class RenderId
	{
		GridRender,
		Geometry,
		Render2D,
		PostProcess,
		Length
	};

	class MAPLE_EXPORT Renderer
	{
	  public:
		virtual ~Renderer()                                                      = default;
		virtual auto init(const std::shared_ptr<GBuffer> &buffer) -> void        = 0;
		virtual auto renderScene() -> void                                       = 0;
		virtual auto beginScene(Scene *scene, const glm::mat4 &projView) -> void = 0;

		virtual auto beginScenePreview(Scene *scene, const glm::mat4 &projView) -> void{};
		virtual auto renderPreviewScene() -> void{};
		virtual auto onResize(uint32_t width, uint32_t height) -> void{};
		virtual auto onImGui() -> void{};

		virtual auto setRenderTarget(std::shared_ptr<Texture> texture, bool rebuildFramebuffer = true) -> void
		{
			renderTexture = texture;
		}

		static auto bindDescriptorSets(Pipeline *pipeline, CommandBuffer *cmdBuffer, uint32_t dynamicOffset, const std::vector<std::shared_ptr<DescriptorSet>> &descriptorSets) -> void;
		static auto drawIndexed(CommandBuffer *commandBuffer, DrawType type, uint32_t count, uint32_t start = 0) -> void;
		static auto drawMesh(CommandBuffer *cmdBuffer, Pipeline *pipeline, Mesh *mesh) -> void;

		auto getCommandBuffer() -> CommandBuffer *;

	  protected:
		uint32_t width;
		uint32_t height;

		std::shared_ptr<Texture> renderTexture;
		std::shared_ptr<GBuffer> gbuffer;
	};
};        // namespace maple
