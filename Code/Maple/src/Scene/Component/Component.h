//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <string>

namespace maple
{
	namespace component 
	{
		struct DeltaTime 
		{
			float dt;
		};

		struct  NameComponent
		{
			std::string name;
		};

		struct StencilComponent
		{
		};

		struct  ActiveComponent
		{
			bool active = true;
		};
	}
};        // namespace maple
