//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "SpriteSystem.h"
#include "Scene/Component/Transform.h"
#include "Sprite.h"
#include <ecs/ecs.h>

namespace maple
{
	namespace update
	{
		using Entity = ecs::Registry ::Modify<component::AnimatedSprite>::Modify<component::Transform>::To<ecs::Entity>;

		inline auto systemAnimatedSprite(Entity entity, const global::component::DeltaTime &dt, ecs::World world)
		{
			auto [anim, trans] = entity;

			anim.frameTimer += dt.dt;
			if (anim.currentFrame < anim.animationFrames.size())
			{
				auto &frame = anim.animationFrames[anim.currentFrame];
				if (anim.frameTimer >= frame.delay)
				{
					anim.frameTimer = 0;
					anim.currentFrame++;
					if (anim.currentFrame >= anim.animationFrames.size())
					{
						anim.currentFrame = anim.loop ? 0 : anim.animationFrames.size() - 1;
					}
				}
			}
		};

		using SpriteEntity = ecs::Registry ::Modify<component::Sprite>::Modify<component::Transform>::To<ecs::Entity>;

		inline auto systemSprite(SpriteEntity entity, ecs::World world){

		};
	};        // namespace update

	namespace sprite2d
	{
		auto registerSprite2dModule(std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerSystem<update::systemAnimatedSprite>();
			executePoint->registerSystem<update::systemSprite>();
		}
	}        // namespace sprite2d
};           // namespace maple