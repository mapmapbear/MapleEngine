//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Engine/Core.h"
#include "Scene/System/ExecutePoint.h"

namespace maple
{
	namespace ddgi
	{
		namespace component
		{
			struct DDGIVisualization
			{
				bool enable = true;
				float scale  = 1.f;
			};
		};        // namespace component

		auto registerDDGIVisualization(ExecuteQueue &begin, ExecuteQueue &render, std::shared_ptr<ExecutePoint> point) -> void;
	};        // namespace ddgi
}        // namespace maple