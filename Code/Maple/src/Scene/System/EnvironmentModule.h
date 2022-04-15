//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <string>
#include "Engine/Core.h"

namespace maple
{
	namespace component
	{
		struct Environment;
	}

	namespace environment 
	{
		auto MAPLE_EXPORT init(component::Environment&, const std::string& fileName) -> void;
	};
};