//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Engine/Material.h"
#include "FileSystem/MeshResource.h"

#include <cereal/types/memory.hpp>
#include <memory>
#include <vector>

namespace maple
{
	class Mesh;
	class MeshResource;
	class Skeleton;

	namespace component
	{
		class Transform;
		enum class PrimitiveType : int32_t
		{
			Plane    = 0,
			Quad     = 1,
			Cube     = 2,
			Pyramid  = 3,
			Sphere   = 4,
			Capsule  = 5,
			Cylinder = 6,
			Terrain  = 7,
			File     = 8,
			Length
		};

		struct SkinnedMeshRenderer
		{
			bool        castShadow = true;
			std::string meshName;
			std::string filePath;

			std::shared_ptr<Mesh>        mesh;
			std::shared_ptr<Skeleton>    skeleton;
			std::shared_ptr<glm::mat4[]> boneTransforms;
		};

		struct BoneComponent
		{
		  public:
			int32_t   boneIndex;
			Skeleton *skeleton;
		};

		struct MeshRenderer
		{
			bool                  castShadow = true;
			bool                  active     = true;
			PrimitiveType         type;
			std::shared_ptr<Mesh> mesh;
			std::string           meshName;
			std::string           filePath;
		};
	}        // namespace component
};           // namespace maple
