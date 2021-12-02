//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <imgui.h>
#include <memory>
#include <string>

#include "EditorWindow.h"
#include "Engine/CameraController.h"
#include "Scene/Component/Transform.h"

namespace maple
{
	class Texture2D;
	class SceneWindow : public EditorWindow
	{
	  public:
		SceneWindow();
		virtual auto onImGui() -> void;
		virtual auto resize(uint32_t width, uint32_t height) -> void;

		auto handleInput(float dt) -> void override;


	  private:
		auto drawToolBar() -> bool;
		auto drawGizmos(float width, float height, float xpos, float ypos, Scene *scene) -> void;

		auto draw2DGrid(ImDrawList *  drawList,
		                const ImVec2 &cameraPos,
		                const ImVec2 &windowPos,
		                const ImVec2 &canvasSize,
		                const float   factor,
		                const float   thickness) -> void;

		bool                       showCamera = false;
		uint32_t                   width;
		uint32_t                   height;
		std::shared_ptr<Texture2D> previewTexture;

	};
};        // namespace maple
