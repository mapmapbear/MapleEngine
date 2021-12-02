//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "EditorWindow.h"
#include <glm/glm.hpp>
#include <imgui.h>
#include <memory>
#include <string>

namespace maple
{
	class Texture2D;
	class TextureDepth;
	class Scene;
	class Transform;

	class PreviewWindow : public EditorWindow
	{
	  public:
		PreviewWindow();
		~PreviewWindow();
		auto onImGui() -> void override;
		auto resize(uint32_t width, uint32_t height) -> void;
		auto handleInput(float dt) -> void override;

		auto handleRotation(float dt, const glm::vec2 &pos) -> void;

	  private:
		std::shared_ptr<Texture2D>    renderTexture;
		std::shared_ptr<TextureDepth> depthTexture;
		Scene *                       scene             = nullptr;
		uint32_t                      width             = 0;
		uint32_t                      height            = 0;
		glm::vec2                     previousCurserPos = glm::vec2(0.0f, 0.0f);
		glm::vec2                     rotateVelocity    = glm::vec2(0.0f, 0.0f);
	};
};        // namespace maple
