//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "AnimationCurve.h"
#include "FileSystem/IResource.h"
#include <memory>
#include <string>
#include <vector>

namespace maple
{
	namespace component
	{
		class Transform;
	}

	enum class AnimationCurvePropertyType
	{
		Unknown = 0,
		LocalPositionX,
		LocalPositionY,
		LocalPositionZ,
		LocalRotationX,
		LocalRotationY,
		LocalRotationZ,
		LocalQuaternionX,
		LocalQuaternionY,
		LocalQuaternionZ,
		LocalQuaternionW,
		LocalScaleX,
		LocalScaleY,
		LocalScaleZ,
		BlendShape
	};

	inline auto getCurveTypeName(AnimationCurvePropertyType type)
	{
		switch (type)
		{
#define STR(r) \
	case r:    \
		return #r
			STR(AnimationCurvePropertyType::Unknown);
			STR(AnimationCurvePropertyType::LocalPositionX);
			STR(AnimationCurvePropertyType::LocalPositionY);
			STR(AnimationCurvePropertyType::LocalPositionZ);
			STR(AnimationCurvePropertyType::LocalRotationX);
			STR(AnimationCurvePropertyType::LocalRotationY);
			STR(AnimationCurvePropertyType::LocalRotationZ);
			STR(AnimationCurvePropertyType::LocalQuaternionX);
			STR(AnimationCurvePropertyType::LocalQuaternionY);
			STR(AnimationCurvePropertyType::LocalQuaternionZ);
			STR(AnimationCurvePropertyType::LocalQuaternionW);
			STR(AnimationCurvePropertyType::LocalScaleX);
			STR(AnimationCurvePropertyType::LocalScaleY);
			STR(AnimationCurvePropertyType::LocalScaleZ);
#undef STR
			default:
				return "UNKNOWN_ERROR";
		}
	}

	struct AnimationCurveProperty
	{
		AnimationCurvePropertyType type;
		std::string                name;
		AnimationCurve             curve;
	};

	struct AnimationCurveWrapper
	{
		std::string                         path;
		int32_t                             boneIndex = -1;
		std::vector<AnimationCurveProperty> properties;
	};

	enum class AnimationWrapMode
	{
		Default      = 0,
		Once         = 1,
		Loop         = 2,
		PingPong     = 4,
		ClampForever = 8,
	};

	struct AnimationClip
	{
		AnimationClip() :
		    length(0),
		    fps(0),
		    wrapMode(AnimationWrapMode::Default)
		{}
		std::string                        name;
		float                              length;
		float                              fps;
		AnimationWrapMode                  wrapMode;
		std::vector<AnimationCurveWrapper> curves;
	};

	enum class FadeState
	{
		In,
		Normal,
		Out,
	};

	class MAPLE_EXPORT Animation : public IResource
	{
	  public:
		Animation(const std::string &filePath) :
		    filePath(filePath)
		{}

		virtual ~Animation() = default;

		inline auto setClips(const std::vector<std::shared_ptr<AnimationClip>> &clips) -> void
		{
			this->clips = clips;
		}
		inline auto addClip(const std::shared_ptr<AnimationClip> &clip) -> void
		{
			clips.emplace_back(clip);
		}

		inline auto getClipName(int32_t index) const
		{
			return clips[index]->name;
		}
		inline auto getClipLength(int32_t index) const
		{
			return clips[index]->length;
		}
		inline auto getClipCount() const
		{
			return clips.size();
		}
		inline auto &getClips() const
		{
			return clips;
		}

		inline auto getResourceType() const -> FileType override
		{
			return FileType::Animation;
		};
		inline auto getPath() const -> std::string
		{
			return filePath;
		}

	  private:
		std::vector<std::shared_ptr<AnimationClip>> clips;
		std::string                                 filePath;
	};
}        // namespace maple