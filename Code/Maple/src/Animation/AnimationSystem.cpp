//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "AnimationSystem.h"
#include "Animator.h"
#include "Scene/Component/Transform.h"
#include "Math/MathUtils.h"
#include <ecs/ecs.h>

namespace maple
{
	namespace animation 
	{
		using Entity = ecs::Chain
			::Write<component::Animator>
			::To<ecs::Entity>;

		inline auto sample(Entity entity, 
			component::Animator & animator, 
			component::Animator::AnimationState & state,
			float time, float weight, bool firstState, bool lastState) -> void
		{
			const auto& clip = *animator.animation->getClips()[state.clipIndex];
			if (state.targets.size() == 0)
			{
				state.targets.resize(clip.curves.size(), nullptr);
			}

			auto currentEntity = entity.castTo<maple::Entity>();

			for (int i = 0; i < clip.curves.size(); ++i)
			{
				const auto& curve = clip.curves[i];
				auto target = state.targets[i];
				if (target == nullptr)
				{
					auto find = currentEntity.findByPath(curve.path);
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

		inline auto system(Entity entity, ecs::World world)
		{
			auto [animator] = entity;
			auto & dt = world.getComponent<component::DeltaTime>();

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

				sample(entity, animator, state, state.playingTime, state.weight, firstState, lastState);

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

		auto registerAnimationSystem(std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerSystem<animation::system>();
		}
	}
};
