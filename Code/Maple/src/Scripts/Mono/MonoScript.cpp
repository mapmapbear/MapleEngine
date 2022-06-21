
//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "MonoScript.h"
#include "IconsMaterialDesignIcons.h"
#include "MapleMonoClass.h"
#include "MapleMonoMethod.h"
#include "MapleMonoObject.h"
#include "MonoComponent.h"
#include "MonoSystem.h"
#include "MonoVirtualMachine.h"
#include "Others/StringUtils.h"
#include "Scene/Component/Transform.h"
#include "Scene/Entity/Entity.h"

namespace maple
{
	MonoScript::MonoScript(const std::string &name, int32_t entity) :
	    name(name)
	{
		className         = StringUtils::getFileNameWithoutExtension(name);
		classNameInEditor = "\t" + className;
		classNameInEditor = ICON_MDI_LANGUAGE_CSHARP + classNameInEditor;
		loadFunction([&](std::shared_ptr<MapleMonoObject> monoObject) {
			monoObject->setValue(&entity, "_internal_entity_handle");
		});
	}

	auto MonoScript::onStart() -> void
	{
		if (startFunc)
		{
			startFunc->invokeVirtual(scriptObject->getRawPtr());
		}
	}

	auto MonoScript::onUpdate(float dt) -> void
	{
		if (updateFunc)
		{
			updateFunc->invokeVirtual(scriptObject->getRawPtr(), dt);
		}
	}

	MonoScript::~MonoScript()
	{
		if (destoryFunc)
		{
			destoryFunc->invokeVirtual(scriptObject->getRawPtr());
		}
	}

	auto MonoScript::loadFunction(const std::function<void(std::shared_ptr<MapleMonoObject>)> &callback) -> void
	{
		auto clazz = MonoVirtualMachine::get()->findClass("", className);
		if (clazz != nullptr)
		{
			scriptObject = clazz->createInstance(false);
			clazz->getAllMethods();
			startFunc   = clazz->getMethodExact("OnStart", "");
			updateFunc  = clazz->getMethodExact("OnUpdate", "single");
			destoryFunc = clazz->getMethodExact("OnDestory", "");
			callback(scriptObject);
			/*	scriptObject->setValue(&component->getEntityId(), "_internal_entity_handle");
			scriptObject->setValue(entity.tryGetComponent<component::Transform>(), "_internal_entity_handle");*/
			scriptObject->construct();
		}
	}
};        // namespace maple