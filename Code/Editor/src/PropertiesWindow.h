//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma  once
#include <string>
#include <imgui.h>
#include <memory>

#include "EditorWindow.h"
#include <ImGuiEnttEditor.hpp>


namespace maple 
{
	
	class PropertiesWindow : public EditorWindow
	{
	public:
		PropertiesWindow();
		virtual auto onImGui() -> void override;
		virtual auto onSceneCreated(Scene* scene) -> void override;

	private:

		auto drawResource(const std::string & path) -> void;

		bool init = false;
		MM::EntityEditor<entt::entity> enttEditor;
	};
};