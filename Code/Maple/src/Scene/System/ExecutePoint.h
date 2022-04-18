//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once


#include "Engine/Core.h"
#include "Engine/Profiler.h"

#include <ecs/SystemBuilder.h>
#include <ecs/World.h>
#include <ecs/TypeList.h>
#include <Scene/Entity/Entity.h>

namespace maple
{
	struct ExecuteQueue
	{
		ExecuteQueue(const std::string& name) :name(name) {};
		std::string name;
		std::vector<std::function<void(entt::registry& )>> jobs;
		std::function<void(ecs::World)> preCall =  [](ecs::World) {};
		std::function<void(ecs::World)> postCall = [](ecs::World) {};
	};

	class MAPLE_EXPORT ExecutePoint
	{
	  public:

		inline ExecutePoint()
			:gameStartQueue("GameStart"),
			gameEndedQueue("GameEnded"),
			updateQueue("Update"),
			imGuiQueue("ImGui"),
			factoryQueue("Factory")
		{
			globalEntity = create("global");
		};

		inline auto registerQueue(ExecuteQueue &queue)
		{
			graph.emplace_back(&queue);
		}

		template <typename Component>
		inline auto registerGlobalComponent(const std::function<void(Component&) > & onInit = nullptr) -> void
		{
			factoryQueue.jobs.emplace_back([=](entt::registry& reg) {
				auto & comp = reg.get_or_emplace<Component>(globalEntity);
				if (onInit != nullptr)
					onInit(comp);
			});
		}

		template <auto System>
		inline auto registerFactorySystem() -> void
		{
			expand(ecs::FunctionConstant<System>{}, factoryQueue);
		}

		template <auto System>
		inline auto registerSystem() -> void
		{
			expand(ecs::FunctionConstant<System>{}, updateQueue);
		}

		template <auto System>
		inline auto registerGameStart() -> void
		{
			expand(ecs::FunctionConstant<System>{}, gameStartQueue);
		}

		template <auto System>
		inline auto registerGameEnded() -> void
		{
			expand(ecs::FunctionConstant<System>{}, gameEndedQueue);
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

		template <typename R, typename ...T>
		inline auto addDependency() -> void
		{
			(registry.on_construct<R>().connect<&entt::registry::get_or_emplace<T>>(), ...);
		}

		auto clear() -> void;
		auto removeAllChildren(entt::entity entity, bool root = true) -> void;
		auto removeEntity(entt::entity entity) -> void;

		auto create()->Entity;
		auto create(const std::string& name)->Entity;
		auto getEntityByName(const std::string& name)->Entity;

		inline auto getGlobalEntity() const { return globalEntity; }

		inline auto& getRegistry()
		{
			return registry;
		}

		template <typename... Components>
		inline auto addGlobalComponent()
		{
			(registry.emplace<Components>(globalEntity), ...);
		}

		template <typename Component, typename... Args>
		inline auto& getGlobalComponent(Args &&...args)
		{
			return registry.template get_or_emplace<Component>(globalEntity,std::forward<Args>(args)...);
		}


		
		template<typename TComponent, auto Candidate>
		inline auto onConstruct(bool connect = true)
		{
			if(connect)
				registry.template on_construct<TComponent>().connect<&delegateComponent<TComponent, Candidate>>(globalEntity);
			else
				registry.template on_construct<TComponent>().disconnect<&delegateComponent<TComponent, Candidate>>(globalEntity);
		}

		template<typename TComponent, auto Candidate>
		inline auto onUpdate(bool connect = true)
		{
			if (connect)
				registry.template on_update<TComponent>().connect<&delegateComponent<TComponent, Candidate>>(globalEntity);
			else
				registry.template on_update<TComponent>().disconnect<&delegateComponent<TComponent, Candidate>>(globalEntity);
		}

		template<typename TComponent, auto Candidate>
		inline auto onDestory(bool connect = true)
		{
			if (connect)
				registry.template on_destroy<TComponent>().connect<&delegateComponent<TComponent, Candidate>>(globalEntity);
			else
				registry.template on_destroy<TComponent>().disconnect<&delegateComponent<TComponent, Candidate>>(globalEntity);
		}

		//these two will be refactored in the future because of the ExecutePoint could belong to scene ?
		inline auto onGameStart() 
		{
			for (auto& job : gameStartQueue.jobs)
			{
				job(registry);
			}
		}

		inline auto onGameEnded()
		{
			for (auto& job : gameEndedQueue.jobs)
			{
				job(registry);
			}
		}

	  private:
		  friend class Application;

		  template<typename TComponent, auto Candidate>
		  static auto delegateComponent(entt::entity globalEntity, entt::registry& registry, entt::entity entity)
		  {
			  auto& comp = registry.get<TComponent>(entity);
			  std::invoke(Candidate, comp, Entity{ entity,registry }, ecs::World{ registry,globalEntity });
		  }

		  inline auto onUpdate(float dt)
		  {
			  for (auto& job : updateQueue.jobs)
			  {
				  job(registry);
			  }
		  }

		  inline auto executeImGui()
		  {
			  for (auto& job : imGuiQueue.jobs)
			  {
				  job(registry);
			  }
		  }

		  inline auto execute()
		  {
			  if (!factoryQueue.jobs.empty())
			  {
				  for (auto& job : factoryQueue.jobs)
				  {
					  job(registry);
				  }
				  factoryQueue.jobs.clear();
			  }

			  for (auto g : graph)
			  {
				  g->preCall(ecs::World{ registry,globalEntity });
				  for (auto& func : g->jobs)
				  {
					  func(registry);
				  }
				  g->postCall(ecs::World{ registry,globalEntity });
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
				PROFILE_SCOPE(reflectStr.c_str());
				call(TSystem{}, reg, globalEntity);
			});
		}

		ExecuteQueue gameStartQueue;

		ExecuteQueue gameEndedQueue;

		ExecuteQueue updateQueue;

		ExecuteQueue imGuiQueue;

		ExecuteQueue factoryQueue;

		std::vector<ExecuteQueue *> graph;

		entt::entity globalEntity = entt::null;

		entt::registry registry;
	};
};        // namespace maple
