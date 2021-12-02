//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma  once
#include <string>
#include <vector>
#include <memory>
#include <imgui.h>
#include <entt/entt.hpp>
#include "EditorWindow.h"

//Extension to C#
namespace maple
{
	class MapleMonoMethod;

	class PluginWindow : public EditorWindow
	{
	public:
		PluginWindow();
		auto onImGui() -> void override;
		std::vector<std::shared_ptr<MapleMonoMethod>> imguiFuncs;
	};
};