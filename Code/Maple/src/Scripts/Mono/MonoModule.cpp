//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "MonoModule.h"
#include "MonoComponent.h"
#include "MonoScript.h"

namespace maple
{
	namespace mono
	{
		auto addScript(component::MonoComponent &comp, const std::string &name, int32_t entity) -> void
		{
			if (comp.scripts.find(name) == comp.scripts.end())
			{
				comp.scripts.emplace(name, std::make_shared<MonoScript>(name, entity));
			}
		}

		auto remove(component::MonoComponent &comp, const std::string &script) -> void
		{
			comp.scripts.erase(script);
		}
	}        // namespace mono
};           // namespace maple
