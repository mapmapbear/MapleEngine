//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Engine/Renderer/Renderer.h"
#include "Scene/System/ExecutePoint.h"
#include "RHI/Texture.h"
#include "Engine/Core.h"
#include <vector>

namespace maple
{
	namespace component
	{
		struct LPVGrid
		{
			std::shared_ptr<Texture3D> lpvGridR;
			std::shared_ptr<Texture3D> lpvGridG;
			std::shared_ptr<Texture3D> lpvGridB;
			
			std::shared_ptr<Texture3D> lpvGeometryVolumeR;
			std::shared_ptr<Texture3D> lpvGeometryVolumeG;
			std::shared_ptr<Texture3D> lpvGeometryVolumeB;

			std::shared_ptr<Texture3D> lpvAccumulatorR;
			std::shared_ptr<Texture3D> lpvAccumulatorG;
			std::shared_ptr<Texture3D> lpvAccumulatorB;

			std::vector<std::shared_ptr<Texture3D>> lpvRs;
			std::vector<std::shared_ptr<Texture3D>> lpvGs;
			std::vector<std::shared_ptr<Texture3D>> lpvBs;

			float gridSize = 32;
			int32_t propagateCount = 8;

			glm::vec3 gridDimension = {};

			float cellSize = 1.f;
			float occlusionAmplifier = 1.0f;
			bool debugAABB = false;
			bool showGeometry = false;
			bool enableIndirect = true;
		};
	};

	namespace light_propagation_volume
	{
		auto MAPLE_EXPORT registerGlobalComponent(std::shared_ptr<ExecutePoint> point) -> void;
		auto registerLPV(ExecuteQueue& begin, ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> executePoint) -> void;
		auto registerLPVDebug(ExecuteQueue& begin, ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> executePoint) -> void;
	};
};        // namespace maple
