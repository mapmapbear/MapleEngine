//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <memory>
#include "Engine/Core.h"
#include "Math/BoundingBox.h"
#include "RHI/Definitions.h"
#include "VoxelBufferId.h"

namespace maple
{
	class ExecutePoint;
	struct ExecuteQueue;

	namespace global::component
	{
		struct VoxelBuffer
		{
			std::array<glm::mat4, 3> viewProj;
			std::array<glm::mat4, 3> viewProjInverse;
			BoundingBox box;

			std::array<	std::shared_ptr<Texture3D>, VoxelBufferId::Length> voxelVolume;
			std::array<	std::shared_ptr<Texture3D>, 6> voxelTexMipmap;
			std::shared_ptr<Texture3D> staticFlag;
			std::shared_ptr<Shader> voxelShader;
			std::vector<std::shared_ptr<DescriptorSet>> descriptors;
			std::vector<RenderCommand> commandQueue;
			std::shared_ptr<Texture2D> colorBuffer;
		};
	}

	namespace component
	{
		struct Voxelization
		{
			constexpr static int32_t voxelDimension = 256;
			constexpr static int32_t voxelVolume = voxelDimension * voxelDimension * voxelDimension;
			bool dirty = false;
			bool injectFirstBounce = true;
			float volumeGridSize;
			float voxelSize;
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