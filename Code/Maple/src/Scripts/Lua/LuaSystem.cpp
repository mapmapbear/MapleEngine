//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "LuaSystem.h"
#include "Scene/Scene.h"
#include "LuaComponent.h"
#include "Application.h"

namespace maple
{
	auto LuaSystem::onInit() -> void
	{
		
	}

	auto LuaSystem::onUpdate(float dt, Scene* scene)-> void
	{
		if (Application::get()->getEditorState() == EditorState::Play) 
		{
			auto view = scene->getRegistry().view<component::LuaComponent>();
			for (auto v : view)
			{
				auto& lua = scene->getRegistry().get<component::LuaComponent>(v);
				lua.onUpdate(dt);
			}
		}
	}

	auto LuaSystem::onImGui() -> void
	{

	}
};