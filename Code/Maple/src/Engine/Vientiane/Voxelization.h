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

	namespace component
	{
		struct Voxelization
		{
			constexpr static int32_t voxelDimension = 256;
			constexpr static int32_t voxelVolume = voxelDimension * voxelDimension * voxelDimension;
			bool dirty = false;
		};

		struct UpdateRadiance 
		{
			int32_t j;
		};
	};

	namespace vxgi 
	{
		auto MAPLE_EXPORT registerGlobalComponent(std::shared_ptr<ExecutePoint> point) -> void;
		auto registerVoxelizer(ExecuteQueue& begin, ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> point) -> void;
	}
};