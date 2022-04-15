//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include <string>
#include <cstdint>
#include <memory>
#include <functional>
#include "Engine/Core.h"

namespace maple 
{
	class MonoSystem;
	class MapleMonoObject;
	class MapleMonoMethod;

	namespace component 
	{
		struct MonoComponent;
	}

	class MAPLE_EXPORT MonoScript final
	{
	public:
		MonoScript(const std::string & name,int32_t entity);
		~MonoScript();
		auto onStart() -> void;
		auto onUpdate(float dt) -> void;
		inline auto getClassName() const { return className; }
		inline auto getClassNameInEditor() const { return classNameInEditor; }
		auto loadFunction(const std::function<void(std::shared_ptr<MapleMonoObject>)> & callback) -> void;

		inline auto getUpdateFunc() { return updateFunc; }
		inline auto getStartFunc() { return startFunc; }
		inline auto getDestoryFunc() { return destoryFunc; }
		inline auto getScriptObject() { return scriptObject; }

	private:

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