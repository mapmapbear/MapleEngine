//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Engine/Renderer/Renderer.h"
#include "Scene/System/ExecutePoint.h"
#include "RHI/Texture.h"

namespace maple
{
	namespace component
	{
		struct LPVGrid
		{
			std::shared_ptr<Texture3D> lpvGridR;
			std::shared_ptr<Texture3D> lpvGridG;
			std::shared_ptr<Texture3D> lpvGridB;
		};
	};

	namespace light_propagation_volume
	{
		auto registerLPV(ExecuteQueue& begin, ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> executePoint) -> void;
	};
};        // namespace maple
