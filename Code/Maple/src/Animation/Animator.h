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
		class Transform;
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

		struct Animator
		{
			float time = 0;
			float seekTo = -1;
			bool paused = false;
			bool stopped = true;
			bool started = false;
			bool rootMotion = false;
			std::vector<AnimationState> states;
			std::shared_ptr<Animation> animation;
		};
	}
}