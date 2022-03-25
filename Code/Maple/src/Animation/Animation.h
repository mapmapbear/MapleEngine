//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <string>
#include <vector>
#include <memory>
#include "FileSystem/IResource.h"
#include "AnimationCurve.h"

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
		LocalScaleX,
		LocalScaleY,
		LocalScaleZ,
		BlendShape
	};

	struct AnimationCurveProperty
	{
		AnimationCurvePropertyType type;
		std::string name;
		AnimationCurve curve;
	};

	struct AnimationCurveWrapper
	{
		std::string path;
		std::vector<AnimationCurveProperty> properties;
	};

	enum class AnimationWrapMode
	{
		Default = 0,
		Once = 1,
		Loop = 2,
		PingPong = 4,
		ClampForever = 8,
	};

	struct AnimationClip
	{
		AnimationClip() :length(0), fps(0), wrapMode(AnimationWrapMode::Default) {}
		std::string name;
		float length;
		float fps;
		AnimationWrapMode wrapMode;
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
		Animation(const std::string& filePath) :
			filePath(filePath){}

		virtual ~Animation() = default;

		inline auto setClips(const std::vector<std::shared_ptr<AnimationClip>>& clips) -> void
		{
			this->clips = clips;
		}
		inline auto addClip(const std::shared_ptr<AnimationClip>& clip) -> void
		{
			clips.emplace_back(clip);
		}

		inline auto getClipName(int32_t index) const { return clips[index]->name; }
		inline auto getClipLength(int32_t index) const { return clips[index]->length; }
		inline auto getClipCount() const { return clips.size(); }
		inline auto& getClips() const { return clips; }

		inline auto getResourceType() const->FileType override { return FileType::Animation; };
		inline auto getPath() const->std::string { return filePath; }
	private:
		std::vector<std::shared_ptr<AnimationClip>> clips;
		std::string filePath;
	};
}