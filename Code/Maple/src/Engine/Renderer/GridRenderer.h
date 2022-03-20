//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <memory>
#include "Scene/System/ExecutePoint.h"

#include <IconsMaterialDesignIcons.h>

namespace maple
{
	namespace component 
	{
		struct GridRender
		{
			constexpr static char* ICON = ICON_MDI_GRID;

			bool enable = true;
		};
	}

	namespace grid_renderer
	{
		auto registerGridRenderer(ExecuteQueue& begin, ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> executePoint) -> void;
	};
};        // namespace maple
