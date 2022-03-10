//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include <string>

namespace maple
{
	class Scene;
	namespace component 
	{
		class LuaComponent;
	}
	class MetaFile 
	{
	public:
		MetaFile() = default;
		auto save(const component::LuaComponent* comp, const std::string& name) const -> void;
		auto load(component::LuaComponent* comp, const std::string& name, Scene* scene) -> void;
	private:
		friend class component::LuaComponent;
	};
};