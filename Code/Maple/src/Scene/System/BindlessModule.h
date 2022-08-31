//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <memory>
#include "RHI/DescriptorPool.h"
#include "RHI/DescriptorSet.h"
#include "RHI/StorageBuffer.h"
#include "RHI/Texture.h"
#include "Engine/Material.h"
#include "Scene/System/ExecutePoint.h"
#include <vector>

namespace maple
{
	constexpr uint32_t MAX_SCENE_MESH_INSTANCE_COUNT    = 1024;
	constexpr uint32_t MAX_SCENE_LIGHT_COUNT            = 300;
	constexpr uint32_t MAX_SCENE_MATERIAL_COUNT         = 4096;
	constexpr uint32_t MAX_SCENE_MATERIAL_TEXTURE_COUNT = MAX_SCENE_MATERIAL_COUNT * 4;

	namespace global::component
	{
		struct RaytracingDescriptor
		{
			StorageBuffer::Ptr  lightBuffer;
			StorageBuffer::Ptr  materialBuffer;
			StorageBuffer::Ptr  transformBuffer;
			DescriptorPool::Ptr descriptorPool;

			DescriptorSet::Ptr sceneDescriptor;
			DescriptorSet::Ptr vboDescriptor;
			DescriptorSet::Ptr iboDescriptor;
			DescriptorSet::Ptr materialDescriptor;
			DescriptorSet::Ptr textureDescriptor;

			std::vector<Texture::Ptr> shaderTextures;

			Material::Ptr defaultMaterial;

			int32_t numLights = 0;

			bool updated = false;
		};
	}        // namespace global::component

	namespace bindless
	{
		auto registerBindlessModule(ExecuteQueue &begin, ExecuteQueue &renderer, std::shared_ptr<ExecutePoint> executePoint) -> void;
		auto registerBindless(std::shared_ptr<ExecutePoint> executePoint) -> void;
	}
}        // namespace maple