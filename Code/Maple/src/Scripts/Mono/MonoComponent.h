//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include <memory>
#include <unordered_map>

namespace maple
{
	class MonoScript;
	namespace component
	{
		struct MonoComponent
		{
			std::unordered_map<std::string, std::shared_ptr<maple::MonoScript>> scripts;
		};
	}        // namespace component
};           // namespace maple
