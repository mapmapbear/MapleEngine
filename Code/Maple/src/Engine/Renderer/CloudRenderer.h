//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Renderer.h"

namespace maple
{
	enum CloudsTextures
	{
		FragColor,
		Bloom,
		Alphaness,
		CloudDistance,
		Length
	};

	class CloudRenderer : public Renderer
	{
	  public:
		CloudRenderer();
		virtual ~CloudRenderer();
		auto init(const std::shared_ptr<GBuffer> &buffer) -> void override;
		auto renderScene(Scene *scene) -> void override;
		auto beginScene(Scene *scene, const glm::mat4 &projView) -> void override;
		auto onResize(uint32_t width, uint32_t height) -> void override;

		auto getTexture(CloudsTextures id) -> std::shared_ptr<Texture>;

	  private:
		auto onImGui() -> void override;

		struct RenderData;
		struct PseudoSkyboxData;
		struct WeatherPass;
		RenderData *      data        = nullptr;
		PseudoSkyboxData *skyData     = nullptr;
		WeatherPass *     weatherPass = nullptr;
	};
}        // namespace maple
