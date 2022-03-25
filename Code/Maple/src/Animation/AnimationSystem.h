//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Scene/System/ExecutePoint.h"
#include "Engine/Core.h"
#include "Animator.h"

namespace maple
{
	namespace animation 
	{
		auto MAPLE_EXPORT setPlayingTime(component::Animator& animator, float time) -> void;
		auto MAPLE_EXPORT play(component::Animator& animator, int32_t index, float fadeLength) -> void;
		auto MAPLE_EXPORT stop(component::Animator& animator) -> void;
		auto MAPLE_EXPORT pause(component::Animator& animator) -> void;

		inline auto isPaused(const component::Animator& animator) { return animator.paused; }
		inline auto isStarted(const component::Animator& animator) { return animator.started; }
		inline auto isPlaying(const component::Animator& animator) { return !animator.paused && !animator.stopped; }
		inline auto isStopped(const component::Animator& animator) { return animator.stopped; }
		inline auto getTime(const component::Animator& animator) { return animator.time; }
		inline auto getPlayingClip(const component::Animator& animator)
		{
			if (animator.states.empty() || animator.stopped)
			{
				return -1;
			}
			return animator.states.back().clipIndex;
		}
		inline auto getPlayingTime(const component::Animator& animator) 
		{
			if (animator.states.empty() || animator.stopped)
			{
				return 0.f;
			}
			return animator.states.back().playingTime;
		}

		auto registerAnimationSystem(std::shared_ptr<ExecutePoint> executePoint) -> void;
	}
};