//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <memory>
#include "Engine/Core.h"
#include "VoxelBufferId.h"
#include <glm/glm.hpp>

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
				VoxelBufferId::Id id = VoxelBufferId::Id::Albedo;
				glm::vec4 colorChannels = { 1,1,1,1 };
				bool drawMipmap = false;
				int32_t mipLevel = 0;
				int32_t direction = 0;
			};
		}
		auto registerVXGIVisualization(ExecuteQueue& begin, ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> point) -> void;
	}
};