//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <string>
#include "Engine/Core.h"

namespace maple 
{
	class LoadingDialog 
	{
	public:
		auto show(const std::string & name = "LoadingDialog")-> void;
		auto close() -> void;
		auto onImGui() -> void;
	private:
		bool active = false;
		std::string name;
	};
};
