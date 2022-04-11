//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "Mono.h"
#include "MonoSystem.h"
#include "MonoHelper.h"
#include "MonoExporter.h"
#include "MonoScript.h"
#include "MonoVirtualMachine.h"

#include "MapleMonoMethod.h"
#include "MapleMonoObject.h"

#include "Scene/Scene.h"
#include "Others/StringUtils.h"
#include "MonoComponent.h"
#include "MonoScriptInstance.h"

#include "Scene/Component/AppState.h"
#include "Scene/Component/Transform.h"
#include "Scene/Entity/Entity.h"

#include <ecs/ecs.h>

namespace maple
{
	namespace update 
	{
		using Entity = ecs::Chain
			::Write<component::MonoComponent>
			::To<ecs::Entity>;

		inline auto system(Entity entity, ecs::World world)
		{
			auto [mono] = entity;
			auto dt = world.getComponent<component::DeltaTime>().dt;

			if (world.getComponent<component::AppState>().state == EditorState::Play)
			{
				for (auto& script : mono.getScripts())
				{
					if (script.second->getUpdateFunc())
					{
						script.second->getUpdateFunc()->invokeVirtual(script.second->getScriptObject()->getRawPtr(), dt);
					}
				}
			}
		}
	};

	namespace mono
	{
		auto callMonoStart(MonoQuery query) -> void
		{
			for (auto entity : query)
			{
				auto [mono] = query.convert(entity);
				for (auto& script : mono.getScripts())
				{
					script.second->onStart();
				}
			}
		}

		auto recompile(MonoQuery query) -> void
		{
			for (auto entity : query)
			{
				auto [mono] = query.convert(entity);
				for (auto& script : mono.getScripts())
				{
					script.second->loadFunction();
				}
			}
		}

		auto registerMonoModule(std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerGlobalComponent<component::MonoEnvironment>([](component::MonoEnvironment&) {
				MonoVirtualMachine::get()->loadAssembly("./", "MapleLibrary.dll");
				//MonoVirtualMachine::get()->loadAssembly("./", "MapleAssembly.dll");
			});
			executePoint->registerSystem<update::system>();
		}
	}
};