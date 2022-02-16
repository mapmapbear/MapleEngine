//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "EditorWindow.h"
#include <entt/entt.hpp>
#include <imgui.h>
#include <memory>
#include <string>

namespace maple
{
	class HierarchyWindow : public EditorWindow
	{
	  public:
		static constexpr char *STATIC_NAME = ICON_MDI_PINE_TREE " Hierarchy";

		HierarchyWindow();
		virtual auto onImGui() -> void override;
		inline auto  isDraging() const
		{
			return draging;
		}

	  private:
		auto drawName() -> void;
		auto popupWindow() -> void;
		auto drawFilter() -> void;
		auto dragEntity() -> void;
		auto drawNode(const entt::entity &node, entt::registry &registry) -> void;
		auto isParentOfEntity(entt::entity entity, entt::entity child, entt::registry &registry) -> bool;

		ImGuiTextFilter hierarchyFilter;
		entt::entity    doubleClickedEntity = entt::null;
		entt::entity    droppedEntity       = entt::null;
		entt::entity    recentDroppedEntity = entt::null;
		bool            draging             = false;
	};
};        // namespace maple
