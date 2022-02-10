//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Engine/Core.h"
#include "ISystem.h"
#include <ecs/SystemBuilder.h>

namespace maple
{
	using ExecuteQueue = std::vector<std::function<void(Scene *)>>;

	class MAPLE_EXPORT ExecutePoint : public ISystem
	{
	  public:
		inline auto registerQueue(ExecuteQueue &queue)
		{
			graph.emplace_back(&queue);
		}

		template <auto System>
		inline auto registerSystem() -> void
		{
			expand(ecs::FunctionConstant<System>{}, updateQueue);
		}

		template <auto System>
		inline auto registerWithinQueue(ExecuteQueue &queue) -> void
		{
			expand(ecs::FunctionConstant<System>{}, queue);
		}

		auto onInit() -> void override
		{}

		auto onUpdate(float dt, Scene *scene) -> void override
		{
			for (auto &job : updateQueue)
			{
				job(scene);
			}
		}

		inline auto execute(Scene *scene)
		{
			for (auto g : graph)
			{
				for (auto &func : *g)
				{
					func(scene);
				}
			}
		}

		auto onImGui() -> void override
		{}


	  private:
		template <auto System>
		inline auto expand(ecs::FunctionConstant<System> system, ExecuteQueue &queue) -> void
		{
			build(system, queue);
		}

		template <typename TSystem>
		inline auto build(TSystem, ExecuteQueue &queue) -> void
		{
			queue.emplace_back([](Scene *scene) {
				auto call = ecs::CallBuilder::template buildCall(TSystem{});
				call(TSystem{}, scene->getRegistry());
			});
		}

		ExecuteQueue updateQueue;

		std::vector<ExecuteQueue *> graph;
	};

};        // namespace maple
