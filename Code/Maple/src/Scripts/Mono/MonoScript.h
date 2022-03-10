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
		MonoScript(const std::string & name, component::MonoComponent * component, MonoSystem* system);
		~MonoScript();
		auto onStart(MonoSystem* system) -> void;
		auto onUpdate(float dt,MonoSystem * system) -> void;
		inline auto getClassName() const { return className; }
		inline auto getClassNameInEditor() const { return classNameInEditor; }
		auto loadFunction() -> void;
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