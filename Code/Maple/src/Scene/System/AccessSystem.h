//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "ISystem.h"
#include <ecs/SystemBuilder.h>

namespace maple
{
	class AccessSystem : public ISystem
	{
	  public:
		
		template <auto System>
		inline auto registerSystem() -> void
		{
			expand(ecs::FunctionConstant<System>{});
		}

		template <auto System>
		inline auto expand(ecs::FunctionConstant<System> system) -> void
		{
			build(system);
		}

		template <typename TSystem>
		inline auto build(TSystem) -> void
		{
			jobs.emplace_back([](Scene *scene) {
				auto call = ecs::CallBuilder::template buildCall(TSystem{});
				call(TSystem {},scene->getRegistry());
			});
		}

		auto onInit() -> void override
		{}

		auto onUpdate(float dt, Scene *scene) -> void override
		{
			for (auto &job : jobs)
			{
				job(scene);
			}
		}

		auto onImGui() -> void override
		{}

	  private:
		std::vector<std::function<void(Scene *)>> jobs;
	};

};        // namespace maple
