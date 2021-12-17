//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Quad2D.h"
#include "Engine/Vertex.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "RHI/DescriptorSet.h"
#include "Renderer.h"

namespace maple
{
	class VertexBuffer;
	class IndexBuffer;
	class Texture;


	class MAPLE_EXPORT Renderer2D : public Renderer
	{
	  public:
		Renderer2D(bool enableDepth = false);
		~Renderer2D();
		auto init(const std::shared_ptr<GBuffer> &buffer) -> void override;
		auto renderScene() -> void override;
		auto beginScene(Scene *scene, const glm::mat4 &projView) -> void override;
		auto setRenderTarget(std::shared_ptr<Texture> texture, bool rebuildFramebuffer) -> void override;

	  private:
		auto present() -> void;
		auto submitTexture(const std::shared_ptr<Texture> &texture) -> int32_t;
		auto flush() -> void;

		struct Renderer2DData;
		Renderer2DData *data = nullptr;
	};
};        // namespace maple
