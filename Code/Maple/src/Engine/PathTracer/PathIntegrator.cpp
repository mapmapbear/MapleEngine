//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "PathIntegrator.h"
#include "RHI/Pipeline.h"
#include "Scene/Component/Light.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Component/Transform.h"

namespace maple
{
	namespace component
	{
		struct PathTracePipeline
		{
			Pipeline::Ptr pipeline;
			Shader::Ptr   shader;
		};
	}        // namespace component

	namespace init
	{
		inline auto initPathIntegrator(component::PathIntegrator &path, Entity entity, ecs::World world)
		{
			auto &pipeline  = entity.addComponent<component::PathTracePipeline>();
			pipeline.shader = Shader::create("shaders/PathTrace/PathTrace.shader");
			PipelineInfo info;
			info.shader       = pipeline.shader;
			pipeline.pipeline = Pipeline::get(info);
		}
	}        // namespace init

	namespace on_begin
	{
		using Entity = ecs::Registry ::To<ecs::Entity>;

		using LightDefine = ecs::Registry ::Modify<component::Light>::Fetch<component::Transform>;

		using LightEntity = LightDefine ::To<ecs::Entity>;

		using Group = LightDefine ::To<ecs::Group>;

		using MeshQuery = ecs::Registry ::Modify<component::MeshRenderer>::Modify<component::Transform>::To<ecs::Group>;

		inline auto system(Entity entity, Group lightQuery, MeshQuery meshQuery, ecs::World world)
		{
		}
	}        // namespace on_begin

	namespace path_integrator
	{
		auto registerPathIntegrator(ExecuteQueue &begin, ExecuteQueue &renderer, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->onConstruct<component::PathIntegrator, init::initPathIntegrator>();
		}
	}        // namespace path_integrator
}        // namespace maple
