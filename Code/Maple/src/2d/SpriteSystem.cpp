//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "SpriteSystem.h"
#include "Sprite.h"
#include "Scene/Component/Transform.h"
#include <ecs/ecs.h>

namespace maple 
{
	namespace update 
	{
		using Entity = ecs::Chain
			::Write<component::AnimatedSprite>
			::Write<component::Transform>
			::To<ecs::Entity>;

		inline auto systemAnimatedSprite(Entity entity, const global::component::DeltaTime & dt, ecs::World world)
		{
			auto [anim, trans] = entity;

			anim.frameTimer += dt.dt;
			if (anim.currentFrame < anim.animationFrames.size())
			{
				auto& frame = anim.animationFrames[anim.currentFrame];
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

		using SpriteEntity = ecs::Chain
			::Write<component::Sprite>
			::Write<component::Transform>
			::To<ecs::Entity>;

		inline auto systemSprite(SpriteEntity entity, ecs::World world)
		{

		};
	};

	namespace sprite2d
	{
		auto registerSprite2dModule(std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerSystem<update::systemAnimatedSprite>();
			executePoint->registerSystem<update::systemSprite>();
		}
	}
};