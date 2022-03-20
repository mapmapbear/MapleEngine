//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <string>
#include <vector>
#include <memory>
#include "FileSystem/IResource.h"
#include "Scene/Entity/Entity.h"
#include "AnimationCurve.h"

namespace maple
{
	class Entity;

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

	struct AnimationState
	{
		int32_t clipIndex;
		float playStartTime;
		std::vector<component::Transform*> targets;
		FadeState fadeState;
		float fadeStartTime;
		float fadeLength;
		float startWeight;
		float weight;
		float playingTime;
	};


	class Animation : public IResource
	{
	public:
		Animation(const std::string & filePath);
		virtual ~Animation();

		auto setClips(const std::vector<std::shared_ptr<AnimationClip>>& clips) -> void;
		auto addClip(const std::shared_ptr<AnimationClip>& clips) -> void;

		auto setPlayingTime(float time) -> void;

		inline auto getPlayingTime() const->float
		{
			if (states.empty() || stopped)
			{
				return 0;
			}
			return states.back().playingTime;
		}

		inline auto getPlayingClip() const
		{
			if (states.empty() || stopped)
			{
				return -1;
			}
			return states.back().clipIndex;
		}
		inline auto getClipName(int32_t index) const { return clips[index]->name; }
		inline auto getClipLength(int32_t index) const { return clips[index]->length; }
		inline auto isPaused() const { return paused; }
		inline auto isStarted() const { return started; }
		inline auto isPlaying() const { return !paused && !stopped; }
		inline auto getStates() const { return states.size(); }

		inline auto isStopped() const { return stopped; }
		inline auto getClipCount() const { return clips.size(); }
		inline auto getTime() const { return time; }

		auto play(int32_t index, const Entity& target, float fadeLength = 0.3f) -> void;
		auto stop() -> void;
		auto pause() -> void;
		auto onUpdate(float dt) -> void;


		inline auto getResourceType() const->FileType override { return FileType::Animation; };
		inline auto getPath() const->std::string { return filePath; }

	private:
		auto updateTime(float dt) -> void;
		auto sample(AnimationState& state, float time, float weight, bool firstState, bool lastState) -> void;

	private:
		std::vector<std::shared_ptr<AnimationClip>> clips;
		std::vector<AnimationState> states;
		float time = 0;
		float seekTo = -1;
		bool paused = false;
		bool stopped = true;
		bool started = false;
		Entity target;
		std::string filePath;
	};
}