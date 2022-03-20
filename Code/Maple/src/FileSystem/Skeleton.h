//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include "Engine/Core.h"
#include "FileSystem/IResource.h"

namespace maple
{
	struct Bone 
	{
		int32_t id;
		int32_t parentIdx;
		std::string name;
		std::vector<int32_t> children;

		glm::vec3 position;
		glm::vec3 rotation;

		glm::vec3 restPosition;
		glm::vec3 restRotation;

		glm::mat4 offsetMatrix;
		glm::mat4 worldTransform;
	};
	
	class MAPLE_EXPORT Skeleton  : public IResource
	{
	public:
		Skeleton(const std::string& name);

		auto createBone(int32_t parentId = -1) ->Bone&;
		auto getBoneIndex(const std::string& name) -> int32_t;
		auto buildRoot() -> void;
		inline auto getRoots() const { return roots; }
		inline auto& getBones() const { return bones; }
		inline auto& getBones() { return bones; }
		inline auto isValidBoneIndex(int32_t index) const { return index < bones.size() && index >= 0; }
		inline auto getResourceType() const->FileType {
			return FileType::Skeleton;
		};
		inline auto getPath() const->std::string { return filePath; }
		auto buildHash() -> void;

		inline auto getHashCode()->size_t { if (hashCode == -1) buildHash(); return hashCode; }

	private:
		std::vector<Bone> bones;
		std::vector<int32_t> roots;
		std::string filePath;
		size_t hashCode = -1;
	};

}