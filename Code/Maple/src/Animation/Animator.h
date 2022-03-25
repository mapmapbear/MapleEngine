//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Scene/Component/Component.h"
#include "Animation.h"

namespace maple 
{
	namespace component 
	{
		class Animator : public Component
		{
		public:

			constexpr static char* ICON = ICON_MDI_ANIMATION;

			inline auto isPaused() const { return paused; }
			inline auto isStarted() const { return started; }
			inline auto isPlaying() const { return !paused && !stopped; }
			inline auto isStopped() const { return stopped; }
			inline auto getTime() const { return time; }
			inline auto getPlayingClip() const
			{
				if (states.empty() || stopped)
				{
					return -1;
				}
				return states.back().clipIndex;
			}
			inline auto getPlayingTime() const
			{
				if (states.empty() || stopped)
				{
					return 0.f;
				}
				return states.back().playingTime;
			}

			auto setPlayingTime(float time) -> void;
			auto play(int32_t index, float fadeLength) -> void;

			auto stop() -> void;
			auto pause() -> void;

			float time = 0;
			float seekTo = -1;
			bool paused = false;
			bool stopped = true;
			bool started = false;

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

			std::vector<AnimationState> states;
			std::shared_ptr<Animation> animation;
		};
	}
}