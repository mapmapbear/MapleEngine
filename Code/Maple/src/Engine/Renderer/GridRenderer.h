//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "Renderer.h"

namespace maple
{
	class UniformBuffer;
	class Mesh;

	class MAPLE_EXPORT GridRenderer : public Renderer
	{
	public:
		GridRenderer() = default;
		GridRenderer(uint32_t width, uint32_t height);
		~GridRenderer();

		auto init(const std::shared_ptr<GBuffer>& buffer) -> void override;
		auto renderScene() -> void override;
		auto renderPreviewScene() -> void override;

		auto beginScene(Scene *scene, const glm::mat4 &projView) -> void override;
		auto beginScenePreview(Scene *scene, const glm::mat4 &projView) -> void override;

		auto onResize(uint32_t width, uint32_t height) -> void override;
		auto setRenderTarget(std::shared_ptr<Texture> texture, bool rebuildFramebuffer) -> void override;

	  private:

		std::shared_ptr<Mesh> quad;

		std::shared_ptr<Shader> gridShader;

		std::shared_ptr<Pipeline> pipeline;

		std::shared_ptr<DescriptorSet> descriptorSet;

	
		struct UniformBufferObject
		{
			glm::vec4 cameraPos;
			float     scale = 500.0f;
			float     res = 1.4f;
			float     maxDistance = 600.0f;
			float     p1;
		}systemBuffer;
	};
};