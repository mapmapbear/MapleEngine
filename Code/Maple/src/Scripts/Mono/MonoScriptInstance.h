//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Mono.h"
#include "Others/Console.h"
#include <unordered_map>
#include <tuple>
#include <string>

namespace maple 
{
	struct MonoScriptInstance 
	{
		MonoScriptInstance() = default;
		MonoScriptInstance(const std::string& ns, const std::string& name, const std::string& assembly, const std::function<void()>& initCallback) 
			:ns(ns),name(name),assembly(assembly),initCallback(initCallback){}

		std::string ns;//namespace
		std::string name;
		std::string assembly;
		std::function<void()> initCallback;
		std::shared_ptr<MapleMonoClass> scriptClass;
		std::shared_ptr<MapleMonoField> thisPtrField;
	};
};