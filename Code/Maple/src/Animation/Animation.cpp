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
		states.clear();
	}

	auto Animation::addClip(const std::shared_ptr<AnimationClip>& clip) -> void
	{
		clips.emplace_back(clip);
	}

	auto Animation::setPlayingTime(float t) -> void 
	{
		auto playingClip = getPlayingClip();
		if (playingClip >= 0)
		{
			t = std::clamp(t, 0.0f, getClipLength(playingClip));
			float offset = t - getPlayingTime();
			seekTo = time + offset;
		}
	}

	auto Animation::play(int32_t index, const Entity& target, float fadeLength) -> void
	{
		started = true;
		this->target = target;
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

	auto Animation::stop() -> void
	{
		paused = false;
		stopped = true;
	}
	auto Animation::pause() -> void
	{
		if (!stopped)
		{
			paused = true;
		}
	}
	auto Animation::onUpdate(float dt) -> void
	{
		updateTime(dt);

		bool firstState = true;

		for (auto i = states.begin(); i != states.end(); )
		{
			auto& state = *i;
			float time = this->getTime() - state.playStartTime;
			const auto& clip = *clips[state.clipIndex];
			bool removeLater = false;

			if (time >= clip.length)
			{
				switch (clip.wrapMode)
				{
				case AnimationWrapMode::Once:
					time = clip.length;
					removeLater = true;
					break;
				case AnimationWrapMode::Loop:
					time = std::fmod(time, clip.length);
					break;
				case AnimationWrapMode::PingPong:
					if ((int)(time / clip.length) % 2 == 1)
					{
						// backward
						time = clip.length - fmod(time, clip.length);
					}
					else
					{
						// forward
						time = fmod(time, clip.length);
					}
					break;
				case AnimationWrapMode::Default:
				case AnimationWrapMode::ClampForever:
					time = clip.length;
					break;
				}

			}
			state.playingTime = time;

			if (stopped)
			{
				state.playingTime = 0;
			}

			float fadeTime = getTime() - state.fadeStartTime;
			switch (state.fadeState)
			{
			case FadeState::In:
				if (fadeTime < state.fadeLength)
				{
					state.weight = MathUtils::lerp(state.startWeight, 1.0f, fadeTime / state.fadeLength);
				}
				else
				{
					state.fadeState = FadeState::Normal;
					state.fadeStartTime = this->getTime();
					state.startWeight = 1.0f;
					state.weight = 1.0f;
				}

				if (stopped)
				{
					state.fadeState = FadeState::Normal;
					state.fadeStartTime = this->getTime();
					state.startWeight = 1.0f;
					state.weight = 1.0f;
				}
				break;
			case FadeState::Normal:
				break;
			case FadeState::Out:
				if (fadeTime < state.fadeLength)
				{
					state.weight = MathUtils::lerp(state.startWeight, 1.0f, fadeTime / state.fadeLength);
				}
				else
				{
					state.weight = 0.0f;
					removeLater = true;
				}

				if (stopped)
				{
					state.weight = 0.0f;
					removeLater = true;
				}
				break;
			}

			bool lastState = false;
			auto j = i;
			if (++j == states.end())
			{
				lastState = true;
			}

			sample(state, state.playingTime, state.weight, firstState, lastState);

			firstState = false;

			if (removeLater)
			{
				i = states.erase(i);

				if (states.empty())
				{
					paused = true;
				}
			}
			else
			{
				++i;
			}
		}

		if (stopped)
		{
			states.clear();
		}
	}

	auto Animation::updateTime(float dt) -> void
	{
		if (seekTo >= 0)
		{
			time = seekTo;
			seekTo = -1;
		}
		else
		{
			if (!paused)
			{
				time += dt;
			}
		}
	}
	auto Animation::sample(AnimationState& state, float time, float weight, bool firstState, bool lastState) -> void
	{
		const auto& clip = *clips[state.clipIndex];
		if (state.targets.size() == 0)
		{
			state.targets.resize(clip.curves.size(), nullptr);
		}

		for (int i = 0; i < clip.curves.size(); ++i)
		{
			const auto& curve = clip.curves[i];
			auto target = state.targets[i];
			if (target == nullptr)
			{
				auto find = this->target.findByPath(curve.path);
				if (find.valid())
				{
					target = find.tryGetComponent<component::Transform>();
					state.targets[i] = target;
				}
				else
				{
					continue;
				}
			}

			glm::vec3 localPos(0);
			glm::vec3 localRot(0);
			glm::vec3 localscale(0);
			bool setPos = false;
			bool setRot = false;
			bool setScale = false;

			for (int j = 0; j < curve.properties.size(); ++j)
			{
				auto type = curve.properties[j].type;
				float value = curve.properties[j].curve.evaluate(time);

				switch (type)
				{
				case AnimationCurvePropertyType::LocalPositionX:
					localPos.x = value;
					setPos = true;
					break;
				case AnimationCurvePropertyType::LocalPositionY:
					localPos.y = value;
					setPos = true;
					break;
				case AnimationCurvePropertyType::LocalPositionZ:
					localPos.z = value;
					setPos = true;
					break;

				case AnimationCurvePropertyType::LocalRotationX:
					localRot.x = value;
					setRot = true;
					break;
				case AnimationCurvePropertyType::LocalRotationY:
					localRot.y = value;
					setRot = true;
					break;
				case AnimationCurvePropertyType::LocalRotationZ:
					localRot.z = value;
					setRot = true;
					break;
				}

				if (setPos)
				{
					glm::vec3 pos;
					if (firstState)
					{
						pos = localPos * weight;
					}
					else
					{
						pos = target->getLocalPosition() + localPos * weight;
					}
					target->setLocalPosition(pos);
				}
				if (setRot)
				{
					glm::vec3 rot;
					if (firstState)
					{
						rot = glm::radians(localRot * weight);
					}
					else
					{
						rot = target->getLocalOrientation() + glm::radians(localRot * weight);
					}
					target->setLocalOrientation(rot);
				}
			}
		}
	}
};