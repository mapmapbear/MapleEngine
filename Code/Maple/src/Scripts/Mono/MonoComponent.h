//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Scene/Component/Component.h"
#include <unordered_map>

namespace maple
{
	class MonoScript;
	class MonoSystem;
	namespace component 
	{
		class MAPLE_EXPORT MonoComponent : public Component
		{
		public:
			constexpr static char* ICON = ICON_MDI_LANGUAGE_CSHARP;

			auto        addScript(const std::string& name, MonoSystem* system) -> void;
			inline auto getScripts() const
			{
				return scripts;
			}
			auto remove(const std::string& script) -> void;

		private:
			std::unordered_map<std::string, std::shared_ptr<MonoScript>> scripts;
		};
	}
};        // namespace maple
