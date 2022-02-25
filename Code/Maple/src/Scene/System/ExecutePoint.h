//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once


#include "Engine/Core.h"
#include "Engine/Profiler.h"

#include <ecs/SystemBuilder.h>
#include <ecs/World.h>

namespace maple
{
	struct ExecuteQueue
	{
		std::string name;
		std::vector<std::function<void(entt::registry& )>> jobs;
		std::function<void(const std::string&, ecs::World)> preCall = [](const std::string&, ecs::World) {};
		std::function<void(const std::string&, ecs::World)> postCall = [](const std::string&, ecs::World) {};
	};

	class ExecutePoint
	{
	  public:
		inline auto registerQueue(ExecuteQueue &queue)
		{
			graph.emplace_back(&queue);
		}

		template <typename Component>
		inline auto registerGlobalComponent(const std::function<void(Component &)> & onInit = nullptr) -> void
		{
			factoryQueue.jobs.emplace_back([=](entt::registry& reg) {
				auto & comp = reg.get_or_emplace<Component>(globalEntity);
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
		inline auto registerOnImGui() -> void
		{
			expand(ecs::FunctionConstant<System>{}, imGuiQueue);
		}

		template <auto System>
		inline auto registerWithinQueue(ExecuteQueue &queue)
		{
			expand(ecs::FunctionConstant<System>{}, queue);
		}

	  private:
		  friend class Application;

		  inline auto setGlobalEntity(entt::entity global)
		  {
			  globalEntity = global;
		  }

		  inline auto onUpdate(float dt, entt::registry& reg)
		  {
			  for (auto& job : updateQueue.jobs)
			  {
				  job(reg);
			  }
		  }

		  inline auto executeImGui(entt::registry& reg)
		  {
			  for (auto& job : imGuiQueue.jobs)
			  {
				  job(reg);
			  }
		  }

		  inline auto execute(entt::registry& reg)
		  {
			  if (!factoryQueue.jobs.empty())
			  {
				  for (auto& job : factoryQueue.jobs)
				  {
					  job(reg);
				  }
				  factoryQueue.jobs.clear();
			  }

			  for (auto g : graph)
			  {
				  for (auto& func : g->jobs)
				  {
					  func(reg);
				  }
			  }
		  }

		template <auto System>
		inline auto expand(ecs::FunctionConstant<System> system, ExecuteQueue &queue) -> void
		{
			build(system, queue);
		}

		template <typename TSystem>
		inline auto build(TSystem, ExecuteQueue &queue) -> void
		{
			queue.jobs.emplace_back([&](entt::registry& reg) {
				auto call = ecs::CallBuilder::template buildCall(TSystem{});
				constexpr auto reflectStr = ecs::CallBuilder::template buildFullCallName(TSystem{});
				std::string str = { reflectStr.data(),reflectStr.size() };
				PROFILE_SCOPE(str.c_str());
				queue.preCall(str, ecs::World{ reg });
				call(TSystem{}, reg);
				queue.postCall(str, ecs::World{ reg });
			});
		}

		ExecuteQueue updateQueue;

		ExecuteQueue imGuiQueue;

		ExecuteQueue factoryQueue;

		std::vector<ExecuteQueue *> graph;

		entt::entity globalEntity = entt::null;
	};
};        // namespace maple
