//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include <string>

namespace maple
{
	class LuaComponent;
	class Scene;
	class MetaFile 
	{
	public:
		MetaFile() = default;
		auto save(const LuaComponent* comp, const std::string& name) const -> void;
		auto load(LuaComponent* comp, const std::string& name, Scene* scene) -> void;
	private:
		friend class LuaComponent;
	};
};