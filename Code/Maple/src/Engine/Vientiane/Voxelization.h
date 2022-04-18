//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <memory>

namespace maple
{
	class ExecutePoint;
	struct ExecuteQueue;

	namespace component
	{
		struct Voxelization
		{
			float volumeGridSize;
			float voxelSize;
			int32_t volumeDimension;
		};
	};

	auto registerVoxelizer(ExecuteQueue& begin, ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> point) -> void;
};