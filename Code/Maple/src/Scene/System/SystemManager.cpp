//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "SystemManager.h"

namespace maple 
{
	auto SystemManager::onUpdate(float dt, Scene* scene) -> void
	{
		for (auto& system : systems)
			system.second->onUpdate(dt, scene);
	}

	auto SystemManager::onImGui()-> void
	{
		for (auto& system : systems)
			system.second->onImGui();
	}
};

