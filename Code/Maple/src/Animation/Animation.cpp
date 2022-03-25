//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "Animation.h"
#include "Scene/Component/Transform.h"
#include "Scene/Entity/Entity.h"
#include "Math/MathUtils.h"

#include <algorithm>

namespace maple
{
	Animation::Animation(const std::string& filePath):
		filePath(filePath)
	{

	}

	Animation::~Animation()
	{
	
	}

	auto Animation::setClips(const std::vector<std::shared_ptr<AnimationClip>>& clips) -> void
	{
		this->clips = clips;
	}

	auto Animation::addClip(const std::shared_ptr<AnimationClip>& clip) -> void
	{
		clips.emplace_back(clip);
	}
};