//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Scene/Component/Component.h"
#include <memory>
#include <unordered_map>

namespace maple
{
	class MonoScript;
	namespace component 
	{
		class MAPLE_EXPORT MonoComponent : public Component
		{
		public:

			auto addScript(const std::string& name) -> void;
			auto remove(const std::string& script) -> void;

			inline auto& getScripts()
			{
				return scripts;
			}
		private:
			std::unordered_map<std::string, std::shared_ptr<maple::MonoScript>> scripts;
		};
	}
};        // namespace maple
