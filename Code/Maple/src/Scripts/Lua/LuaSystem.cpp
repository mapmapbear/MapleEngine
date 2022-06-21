//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "LuaSystem.h"
#include "LuaComponent.h"
#include "Application.h"
#include <ecs/ecs.h>

namespace maple
{
	namespace update 
	{
		using Entity = ecs::Registry
			::Fetch<component::LuaComponent>
			::To<ecs::Entity>;

		inline auto system(Entity entity,ecs::World world) 
		{
			auto [luaComp] = entity;
			auto dt = world.getComponent<global::component::DeltaTime>().dt;
			
			if (luaComp.onUpdateFunc && luaComp.onUpdateFunc->isFunction())
			{
				try
				{
					(*luaComp.onUpdateFunc)(*luaComp.table, dt);
				}
				catch (const std::exception& e)
				{
					LOGE("{0}", e.what());
				}
			}
		}
	}

	namespace lua
	{
		auto registerLuaSystem(std::shared_ptr<ExecutePoint> executePoint) -> void 
		{
			executePoint->registerSystem<update::system>();
		}
	}
};