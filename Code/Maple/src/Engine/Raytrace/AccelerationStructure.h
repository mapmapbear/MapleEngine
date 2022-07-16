//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Scene/System/ExecutePoint.h"
#include "Engine/Core.h"

namespace maple
{
	class AccelerationStructure;

	namespace raytracing
	{
		namespace global::component
		{
			struct TopLevelAs
			{
				std::shared_ptr<AccelerationStructure> topLevelAs;
			};
		}

		auto registerAccelerationStructureModule(ExecuteQueue &begin, std::shared_ptr<ExecutePoint> point) -> void;
	}
}        // namespace maple