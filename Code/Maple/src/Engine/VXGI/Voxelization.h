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

	namespace vxgi 
	{
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
				bool enableIndirect = true;
				float volumeGridSize;//the max value in aabb   glm::max(axisSize.x, glm::max(axisSize.y, axisSize.z));
				float voxelSize;
				float maxTracingDistance = 1.0f;
				float aoFalloff = 725.f;
				float samplingFactor = 0.5f;
				float bounceStrength = 1.f;
				float aoAlpha = 0.01;
				float traceShadowHit = 0.5f;
			};

			struct UpdateRadiance
			{
				int32_t j;
			};
		};

		auto MAPLE_EXPORT registerGlobalComponent(std::shared_ptr<ExecutePoint> point) -> void;
		auto registerVoxelizer(ExecuteQueue& begin, ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> point) -> void;
		auto registerUpdateRadiace(ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> point) -> void;
		auto registerVXGIIndirectLighting(ExecuteQueue& begin, std::shared_ptr<ExecutePoint> point) -> void;
	}
};