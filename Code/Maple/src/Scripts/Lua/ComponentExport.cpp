//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "ComponentExport.h"

#include "LuaVirtualMachine.h"
extern "C" {
# include "lua.h"
# include "lauxlib.h"
# include "lualib.h"
}
#include <LuaBridge/LuaBridge.h>
#include <string>
#include <functional>

#include "Scene/Component/Transform.h"
#include "Scene/Component/Component.h"

#include "LuaComponent.h"

#include "Scene/Entity/Entity.h"


namespace maple
{
#define EXPORT_COMPONENTS(Comp) \
		 addFunction("add" #Comp,&Entity::addComponent<Comp>) \
		.addFunction("remove" #Comp, &Entity::removeComponent<Comp>) \
		.addFunction("get" #Comp, &Entity::getComponent<Comp>) \
		.addFunction("getOrAdd" #Comp, &Entity::getOrAddComponent<Comp>) \
		.addFunction("tryGet" #Comp, &Entity::tryGetComponent<Comp>) \
        .addFunction("has" #Comp, &Entity::hasComponent<Comp>) \

	namespace ComponentExport
	{
		auto exportLua(lua_State* L) -> void
		{
			luabridge::getGlobalNamespace(L)
				.beginNamespace("entt")
				.beginClass<entt::registry>("registry")
				.endClass()
				.beginClass<entt::entity>("entity")
				.endClass()
				.endNamespace()


				.beginClass <Entity>("Entity")
				//.addConstructor <void (*) (entt::entity, entt::registry&)>()
				.addConstructor <void (*) ()>()
				.addFunction("valid", &Entity::valid)
				.addFunction("destroy", &Entity::destroy)
				.addFunction("setParent", &Entity::setParent)
				.addFunction("getParent", &Entity::getParent)
				.addFunction("isParent", &Entity::isParent)
				.addFunction("getChildren", &Entity::getChildren)
				.addFunction("setActive", &Entity::setActive)
				.addFunction("isActive", &Entity::isActive)

				.EXPORT_COMPONENTS(component::NameComponent)
				.EXPORT_COMPONENTS(component::ActiveComponent)
				.EXPORT_COMPONENTS(component::Transform)
				.EXPORT_COMPONENTS(component::LuaComponent)

				.endClass()
/*

				.beginClass<EntityManager>("EntityManager")
				.addFunction("Create", static_cast<Entity(EntityManager::*)()> (&EntityManager::create))
				.addFunction("getRegistry", &EntityManager::getRegistry)
				.endClass()*/

				.beginClass<component::NameComponent>("NameComponent")
				.addProperty("name", &component::NameComponent::name)
				.addFunction("getEntity", &component::NameComponent::getEntity)
				.endClass()


				.beginClass<component::ActiveComponent>("ActiveComponent")
				.addProperty("active", &component::ActiveComponent::active)
				.addFunction("getEntity", &component::ActiveComponent::getEntity)
				.endClass()

				.beginClass<component::LuaComponent>("LuaComponent")
				.endClass();

	

		}
	};
};