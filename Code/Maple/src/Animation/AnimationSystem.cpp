//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "AnimationSystem.h"
#include "Animator.h"
#include "Animation.h"

#include "Scene/Entity/Entity.h"
#include "Scene/Component/Transform.h"
#include "Scene/Component/MeshRenderer.h"

#include "Math/MathUtils.h"
#include "Others/Randomizer.h"

#include <algorithm>
#include <ecs/ecs.h>

namespace maple
{
	namespace animation 
	{
		using Entity = ecs::Chain
			::Write<component::Animator>
			::To<ecs::Entity>;

		inline auto findByBoneIndex(maple::Entity entity, int32_t boneIdx) -> maple::Entity
		{
			std::vector<maple::Entity> entites;
			entity.flatChildren<component::BoneComponent>(entites);
			for (auto ent : entites)
			{
				auto & bone = ent.getComponent<component::BoneComponent>();
				if (bone.boneIndex == boneIdx)
				{
					return ent;
				}
			}
			return {};
		}

		inline auto sample(Entity entity, 
			component::Animator & animator, 
			component::AnimationState & state,
			float time, float weight, bool firstState, bool lastState, bool rootMotion) -> void
		{
			const auto& clip = *animator.animation->getClips()[state.clipIndex];
			if (state.targets.size() == 0)
			{
				state.targets.resize(clip.curves.size(), nullptr);
			}

			auto currentEntity = entity.castTo<maple::Entity>();

			for (int32_t i = 0; i < clip.curves.size(); ++i)
			{
				const auto& curve = clip.curves[i];
				auto target = state.targets[i];
				if (target == nullptr)
				{
					maple::Entity find;

					if (curve.boneIndex == -1)
					{
						find = currentEntity.findByPath(curve.path);
					}
					else 
					{
						find = findByBoneIndex(currentEntity, curve.boneIndex);
					}

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
				glm::quat localQuat = glm::identity<glm::quat>();

				bool setPos = false;
				bool setRot = false;
				bool setRotQuat = false;
				bool setScale = false;

				for (int32_t j = 0; j < curve.properties.size(); ++j)
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

					case AnimationCurvePropertyType::LocalQuaternionW:
						localQuat.w = value;
						setRotQuat = true;
						break;
					case AnimationCurvePropertyType::LocalQuaternionX:
						localQuat.x = value;
						setRotQuat = true;
						break;
					case AnimationCurvePropertyType::LocalQuaternionY:
						localQuat.y = value;
						setRotQuat = true;
						break;
					case AnimationCurvePropertyType::LocalQuaternionZ:
						localQuat.z = value;
						setRotQuat = true;
						break;
					}

					if (setPos && !rootMotion)
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

					if (setRotQuat) 
					{
						glm::vec3 rot;
						if (firstState)
						{
							rot = glm::eulerAngles(localQuat) * weight;
						}
						else
						{
							rot = target->getLocalOrientation() + glm::eulerAngles(localQuat) * weight;
						}
						target->setLocalOrientation(rot);
					}
				}
			}
		}

		inline auto system(Entity entity, ecs::World world, const global::component::DeltaTime & dt)
		{
			auto [animator] = entity;

			if (animator.seekTo >= 0)
			{
				animator.time = animator.seekTo;
				animator.seekTo = -1;
			}
			else
			{
				if (!animator.paused)
				{
					animator.time += dt.dt;
				}
			}

			bool firstState = true;

			for (auto i = animator.states.begin(); i != animator.states.end(); )
			{
				auto& state = *i;
				float time = animator.time - state.playStartTime;
				const auto& clip = *animator.animation->getClips()[state.clipIndex];
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

				if (animator.stopped)
				{
					state.playingTime = 0;
				}

				float fadeTime = animator.time - state.fadeStartTime;
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
						state.fadeStartTime = animator.time;
						state.startWeight = 1.0f;
						state.weight = 1.0f;
					}

					if (animator.stopped)
					{
						state.fadeState = FadeState::Normal;
						state.fadeStartTime = animator.time;
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

					if (animator.stopped)
					{
						state.weight = 0.0f;
						removeLater = true;
					}
					break;
				}

				bool lastState = false;
				auto j = i;
				if (++j == animator.states.end())
				{
					lastState = true;
				}

				sample(entity, animator, state, state.playingTime, state.weight, firstState, lastState, animator.rootMotion);

				firstState = false;

				if (removeLater)
				{
					i = animator.states.erase(i);

					if (animator.states.empty())
					{
						animator.paused = true;
					}
				}
				else
				{
					++i;
				}
			}

			if (animator.stopped)
			{
				animator.states.clear();
			}
		}

		auto setPlayingTime(component::Animator& animator, float t) -> void
		{
			auto playingClip = getPlayingClip(animator);
			if (playingClip >= 0)
			{
				t = std::clamp(t, 0.0f, animator.animation->getClipLength(playingClip));
				float offset = t - getPlayingTime(animator);
				animator.seekTo = animator.time + offset;
			}
		}

		auto play(component::Animator& animator, int32_t index, float fadeLength) -> void
		{
			animator.started = true;
			if (animator.paused)
			{
				animator.paused = false;
				if (index == getPlayingClip(animator))
				{
					return;
				}
			}

			if (animator.states.empty())
			{
				fadeLength = 0;
			}

			if (fadeLength > 0.0f)
			{
				for (auto& state : animator.states)
				{
					state.fadeState = FadeState::Out;
					state.fadeStartTime = getTime(animator);
					state.fadeLength = fadeLength;
					state.startWeight = state.weight;
				}
			}
			else
			{
				animator.states.clear();
			}

			component::AnimationState state;
			state.clipIndex = index;
			state.playStartTime = getTime(animator);
			state.fadeStartTime = getTime(animator);
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
			animator.states.emplace_back(state);
			animator.paused = false;
			animator.stopped = false;
		}

		auto stop(component::Animator& animator) -> void
		{
			animator.paused = false;
			animator.stopped = true;
		}

		auto pause(component::Animator& animator) -> void
		{
			if (!animator.stopped)
			{
				animator.paused = true;
			}
		}

		auto registerAnimationModule(std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerSystem<animation::system>();
		}
	}
};
