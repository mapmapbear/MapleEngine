//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <imgui.h>
#include <memory>
#include <string>

#include "EditorWindow.h"
#include <ImGuiEnttEditor.hpp>

namespace maple
{
	class PropertiesWindow : public EditorWindow
	{
	  public:
		static constexpr char *STATIC_NAME = ICON_MDI_SETTINGS "Inspector";

		PropertiesWindow();
		virtual auto onImGui() -> void override;
		virtual auto onSceneCreated(Scene *scene) -> void override;

	  private:
		auto drawResource(const std::string &path) -> void;

		bool                           init = false;
		MM::EntityEditor<entt::entity> enttEditor;
	};
};        // namespace maple
