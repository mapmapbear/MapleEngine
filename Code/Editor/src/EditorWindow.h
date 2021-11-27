//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <string>

namespace Maple
{
	class Scene;

	class EditorWindow
	{
	  public:
		virtual ~EditorWindow()        = default;
		virtual auto onImGui() -> void = 0;
		virtual auto onSceneCreated(Scene *scene) -> void{};
		virtual auto handleInput(float dt) -> void{};

	  public:
		inline auto &getTitle() const
		{
			return title;
		}
		inline auto setTitle(std::string title)
		{
			this->title = title;
		}

		inline auto isActive() const
		{
			return active;
		}
		inline auto setActive(bool active)
		{
			this->active = active;
		}

		inline auto isFocused() const
		{
			return focused;
		}
		inline auto setFocused(bool focused)
		{
			this->focused = focused;
		}

	  protected:
		std::string title;
		bool        active  = false;
		bool        focused = false;
	};
};        // namespace Maple
