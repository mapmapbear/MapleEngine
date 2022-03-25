//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "Animator.h"
#include "Math/MathUtils.h"
#include "Scene/Component/Transform.h"
#include <algorithm>

namespace maple 
{
	namespace component 
	{
		auto Animator::setPlayingTime(float t) -> void
		{
			auto playingClip = getPlayingClip();
			if (playingClip >= 0)
			{
				t = std::clamp(t, 0.0f, animation->getClipLength(playingClip));
				float offset = t - getPlayingTime();
				seekTo = time + offset;
			}
		}

		auto Animator::play(int32_t index, float fadeLength) -> void
		{
			started = true;
			if (paused)
			{
				paused = false;
				if (index == getPlayingClip())
				{
					return;
				}
			}

			if (states.empty())
			{
				fadeLength = 0;
			}

			if (fadeLength > 0.0f)
			{
				for (auto& state : states)
				{
					state.fadeState = FadeState::Out;
					state.fadeStartTime = this->getTime();
					state.fadeLength = fadeLength;
					state.startWeight = state.weight;
				}
			}
			else
			{
				states.clear();
			}

			AnimationState state;
			state.clipIndex = index;
			state.playStartTime = getTime();
			state.fadeStartTime = getTime();
			state.fadeLength = fadeLength;
			if (fadeLength > 0.0f)
			{
				state.fadeState = FadeState::In;
				state.startWeight = 0.0f;
				state.weight = 0.0f;
			}
			else
			{
				state.fadeState = FadeState::Normal;
				state.startWeight = 1.0f;
				state.weight = 1.0f;
			}
			state.playingTime = 0.0f;
			states.emplace_back(state);
			paused = false;
			stopped = false;
		}

		auto Animator::stop() -> void
		{
			paused = false;
			stopped = true;
		}

		auto Animator::pause() -> void
		{
			if (!stopped)
			{
				paused = true;
			}
		}
	}
}