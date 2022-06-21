//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Scene/System/ExecutePoint.h"
#include <memory>

#include <IconsMaterialDesignIcons.h>

namespace maple
{
	namespace component
	{
		struct GridRender
		{
			bool enable = true;
		};
	}        // namespace component

	namespace grid_renderer
	{
		auto registerGridRenderer(ExecuteQueue &begin, ExecuteQueue &renderer, std::shared_ptr<ExecutePoint> executePoint) -> void;
	};
};        // namespace maple
