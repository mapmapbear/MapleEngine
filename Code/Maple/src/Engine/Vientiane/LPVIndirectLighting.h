//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Engine/Renderer/Renderer.h"
#include "Scene/System/ExecutePoint.h"

namespace maple
{
	namespace lpv_indirect_lighting
	{
		auto registerLPVIndirectLight(ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> executePoint) -> void;
	};
}