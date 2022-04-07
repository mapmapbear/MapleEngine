//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Component.h"
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
			Plane = 0,
			Quad = 1,
			Cube = 2,
			Pyramid = 3,
			Sphere = 4,
			Capsule = 5,
			Cylinder = 6,
			Terrain = 7,
			File = 8,
			Length
		};

		class MAPLE_EXPORT Model final : public Component
		{
		  public:
			Model() = default;
			Model(const std::string& file);

			template <class Archive>
			auto save(Archive& archive) const -> void
			{
				archive(filePath, type, entity);
			}

			template <class Archive>
			auto load(Archive& archive) -> void
			{
				archive(filePath, type, entity);
				load();
			}

			std::string                   filePath;
			PrimitiveType                 type = PrimitiveType::Length;
			std::shared_ptr<MeshResource> resource;
			std::vector<std::shared_ptr<IResource>> resources;
			std::shared_ptr<Skeleton> skeleton;
		  private:
			auto load() -> void;
		};

		class MAPLE_EXPORT SkinnedMeshRenderer : public Component 
		{
		public:
			SkinnedMeshRenderer(const std::shared_ptr<Mesh>& mesh);
			SkinnedMeshRenderer() = default;

			template <class Archive>
			inline auto save(Archive& archive) const -> void
			{
			}

			template <class Archive>
			inline auto load(Archive& archive) -> void
			{
			}

			inline auto& getMesh()
			{
				if (mesh == nullptr)
					getMesh(meshName);
				return mesh;
			}

			auto isActive() const -> bool;

			inline auto isCastShadow() const { return castShadow; }

			inline auto setCastShadow(bool shadow) { castShadow = shadow; }

			bool castShadow = true;

			inline auto getSkeleton() { return skeleton; }

			inline auto setBoneTransform(std::shared_ptr<glm::mat4[]> transforms) 
			{
				boneTransforms = transforms;
			}

			inline auto getBoneTransforms() 
			{
				return boneTransforms;
			}

		private:
			std::shared_ptr<Mesh>     mesh;
			auto                      getMesh(const std::string& name) -> void;
			std::string               meshName;
			std::shared_ptr<Skeleton> skeleton;
			std::shared_ptr<glm::mat4[]> boneTransforms;
		};

		class MAPLE_EXPORT BoneComponent : public Component
		{
		public:

			int32_t boneIndex;
			Skeleton* skeleton;
		};

		class MAPLE_EXPORT MeshRenderer : public Component
		{
		public:

			MeshRenderer() = default;
			MeshRenderer(const std::shared_ptr<Mesh>& mesh);

			template <class Archive>
			inline auto save(Archive& archive) const -> void
			{
			}

			template <class Archive>
			inline auto load(Archive& archive) -> void
			{
			}

			inline auto& getMesh()
			{
				if (mesh == nullptr)
					getMesh(meshName);
				return mesh;
			}

			auto isActive() const -> bool;
			
			inline auto isCastShadow() const { return castShadow; }

			inline auto setCastShadow(bool shadow)  { castShadow = shadow; }

			bool castShadow = true;

		private:
			std::shared_ptr<Mesh>     mesh;
			auto                      getMesh(const std::string& name) -> void;
			std::string               meshName;
		};
	}
};        // namespace maple
