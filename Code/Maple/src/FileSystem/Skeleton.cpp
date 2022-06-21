//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "Skeleton.h"
#include "Engine/Core.h"
#include "Others/Console.h"
#include "Others/HashCode.h"

#include <algorithm>

namespace maple
{
	Skeleton::Skeleton(const std::string &name) :
	    filePath(name)
	{
	}

	auto Skeleton::createBone(int32_t parentId) -> Bone &
	{
		auto &bone     = bones.emplace_back();
		bone.id        = bones.size() - 1;
		bone.parentIdx = parentId;
		return bone;
	}

	auto Skeleton::getBoneIndex(const std::string &name) -> int32_t
	{
		MAPLE_ASSERT(name != "", "name should be null");
		for (auto &bone : bones)
		{
			if (bone.name == name)
				return bone.id;
		}
		return -1;
	}

	auto Skeleton::buildRoot() -> void
	{
		for (auto &bone : bones)
		{
			if (bone.parentIdx == -1)
			{
				root = bone.id;
				break;
			}
		}
	}

	auto Skeleton::buildHash() -> void
	{
		auto copyBones = bones;

		std::sort(copyBones.begin(), copyBones.end(), [](const auto &left, const auto &right) {
			size_t seed1;
			size_t seed2;
			HashCode::hashCode(seed1, left.name);
			HashCode::hashCode(seed2, right.name);
			return seed1 < seed2;
		});

		for (auto &bone : copyBones)
		{
			HashCode::hashCode(hashCode, bone.name);
		}
	}

}        // namespace maple