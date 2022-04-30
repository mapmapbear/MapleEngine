//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <memory>
#include "Engine/Core.h"

namespace maple
{
	class ExecutePoint;
	struct ExecuteQueue;
	namespace vxgi_debug
	{
		namespace global::component 
		{
			struct DrawVoxelRender 
			{
				bool enable = false;
			};
		}

		auto registerVXGIVisualization(ExecuteQueue& begin, ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> point) -> void;
	}
};