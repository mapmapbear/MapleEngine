//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Scene/System/ExecutePoint.h"
#include "Engine/Core.h"

namespace maple
{
	namespace ddgi 
	{
		enum class RaytracingScale
		{
			Full,
			Half,
			Quarter
		};

		auto registerDDGI(ExecuteQueue& begin, std::shared_ptr<ExecutePoint> point) -> void;
	}
}