//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Scene/System/ISystem.h"

namespace maple 
{
	class Scene;
	class MAPLE_EXPORT LuaSystem final : public ISystem
	{
	public:
		auto onInit() -> void override;
		auto onUpdate(float dt, Scene* scene) -> void override;
		auto onImGui() -> void override;
	};
};