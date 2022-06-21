//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "MonoSystem.h"
#include "Mono.h"
#include "MonoExporter.h"
#include "MonoHelper.h"
#include "MonoScript.h"
#include "MonoVirtualMachine.h"

#include "MapleMonoMethod.h"
#include "MapleMonoObject.h"

#include "MonoComponent.h"
#include "MonoScriptInstance.h"
#include "Others/StringUtils.h"
#include "Scene/Scene.h"

#include "Scene/Component/AppState.h"
#include "Scene/Component/Transform.h"
#include "Scene/Entity/Entity.h"

#include <ecs/ecs.h>

namespace maple
{
	namespace update
	{
		using Entity = ecs::Registry ::Modify<component::MonoComponent>::To<ecs::Entity>;

		inline auto system(Entity entity, const global::component::DeltaTime &dt, ecs::World world)
		{
			auto [mono] = entity;
			if (world.getComponent<global::component::AppState>().state == EditorState::Play)
			{
				for (auto &script : mono.scripts)
				{
					if (script.second->getUpdateFunc())
					{
						script.second->getUpdateFunc()->invokeVirtual(script.second->getScriptObject()->getRawPtr(), dt.dt);
					}
				}
			}
		}
	};        // namespace update

	namespace mono
	{
		auto callMonoStart(MonoQuery query) -> void
		{
			for (auto entity : query)
			{
				auto [mono] = query.convert(entity);
				for (auto &script : mono.scripts)
				{
					script.second->onStart();
				}
			}
		}

		auto recompile(MonoQuery query) -> void
		{
			for (auto entity : query)
			{
				auto ent    = query.convert(entity);
				auto [mono] = ent;
				for (auto &script : mono.scripts)
				{
					script.second->loadFunction([&](std::shared_ptr<MapleMonoObject> scriptObject) {
						scriptObject->setValue(&entity, "_internal_entity_handle");
					});
				}
			}
		}

		auto registerMonoModule(std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerGlobalComponent<component::MonoEnvironment>([](component::MonoEnvironment &) {
				MonoVirtualMachine::get()->loadAssembly("./", "MapleLibrary.dll");
				//MonoVirtualMachine::get()->loadAssembly("./", "MapleAssembly.dll");
			});
			executePoint->registerSystem<update::system>();
		}
	}        // namespace mono
};           // namespace maple