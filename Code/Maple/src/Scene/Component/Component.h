//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <string>

namespace maple
{
	namespace component
	{
		struct NameComponent
		{
			std::string name;
		};

		struct StencilComponent
		{
		};

		struct ActiveComponent
		{
			bool active = true;
		};
	}        // namespace component

	namespace global::component
	{
		struct DeltaTime
		{
			float dt;
		};
	}        // namespace global::component
};           // namespace maple
