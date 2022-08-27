//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "RaytracedReflection.h"
#include "Engine/Renderer/RendererData.h"

namespace maple
{
	namespace raytraced_reflection
	{
		namespace component
		{

		}

		namespace delegates
		{
			inline auto init(raytraced_reflection::component::RaytracedReflecton &reflection, Entity entity, ecs::World world)
			{
				auto &winSize     = world.getComponent<maple::component::WindowSize>();
				reflection.width  = winSize.width;
				reflection.height = winSize.height;
			}
		}        // namespace delegates

		auto registerRaytracedReflection(ExecuteQueue &update, ExecuteQueue &queue, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->onConstruct<raytraced_reflection::component::RaytracedReflecton, delegates::init>();
		}
	};        // namespace raytraced_reflection
};            // namespace maple