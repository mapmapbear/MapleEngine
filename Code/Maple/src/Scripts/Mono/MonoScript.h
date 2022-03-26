//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include <string>
#include <cstdint>
#include <memory>
#include "Engine/Core.h"

namespace maple 
{
	class MonoSystem;
	class MapleMonoObject;
	class MapleMonoMethod;

	namespace component 
	{
		class MonoComponent;
	}

	class MAPLE_EXPORT MonoScript final
	{
	public:
		MonoScript(const std::string & name, component::MonoComponent * component);
		~MonoScript();
		auto onStart() -> void;
		auto onUpdate(float dt) -> void;
		inline auto getClassName() const { return className; }
		inline auto getClassNameInEditor() const { return classNameInEditor; }
		auto loadFunction() -> void;

		inline auto getUpdateFunc() { return updateFunc; }
		inline auto getStartFunc() { return startFunc; }
		inline auto getDestoryFunc() { return destoryFunc; }
		inline auto getScriptObject() { return scriptObject; }

	private:

		component::MonoComponent* component = nullptr;
		uint32_t id = 0;
		std::string name;
		std::string className;
		std::string classNameInEditor;
		std::shared_ptr<MapleMonoObject> scriptObject;

		std::shared_ptr<MapleMonoMethod> updateFunc;
		std::shared_ptr<MapleMonoMethod> startFunc;
		std::shared_ptr<MapleMonoMethod> destoryFunc;
	};
};