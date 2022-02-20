//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Engine/Core.h"
#include "ISystem.h"
#include <ecs/SystemBuilder.h>
#include "Engine/Profiler.h"

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

		template <typename Component>
		inline auto registerGlobalComponent(const std::function<void(Component &)> & onInit = nullptr) -> void
		{
			factoryQueue.emplace_back([=](Scene *scene) {
				auto & comp = scene->template getGlobalComponent<Component>();
				if (onInit != nullptr)
					onInit(comp);
			});
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

		inline auto onInit() -> void override
		{
		}

		inline auto onUpdate(float dt, Scene *scene) -> void override
		{
			for (auto &job : updateQueue)
			{
				job(scene);
			}
		}

		inline auto execute(Scene *scene)
		{
			if (!factoryQueue.empty())
			{
				for (auto &job : factoryQueue)
				{
					job(scene);
				}
			}
			factoryQueue.clear();

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
				PROFILE_SCOPE(##TSystem);
				call(TSystem{}, scene->getRegistry());
			});
		}

		ExecuteQueue updateQueue;

		ExecuteQueue factoryQueue;

		std::vector<ExecuteQueue *> graph;
	};

};        // namespace maple
