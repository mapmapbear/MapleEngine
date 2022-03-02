//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "LightPropagationVolume.h"
#include "ReflectiveShadowMap.h"
#include "Scene/Component/BoundingBox.h"
#include "Math/BoundingBox.h"

#include "Engine/Renderer/RendererData.h"
#include "Engine/Mesh.h"

#include "RHI/Texture.h"
#include "RHI/IndexBuffer.h"
#include "RHI/VertexBuffer.h"
#include "RHI/Shader.h"
#include "RHI/Pipeline.h"
#include "RHI/DescriptorSet.h"
#include "Math/BoundingBox.h"
#include <ecs/ecs.h>

namespace maple
{
	namespace 
	{
		inline auto updateGrid(component::LPVGrid& grid, maple::BoundingBox * box) 
		{
			auto dimension = box->size();
			if (grid.lpvGridB == nullptr) 
			{
				grid.lpvGridR = Texture3D::create(dimension.x * 4, dimension.y, dimension.z, TextureFormat::R32I);
				grid.lpvGridB = Texture3D::create(dimension.x * 4, dimension.y, dimension.z, TextureFormat::R32I);
				grid.lpvGridG = Texture3D::create(dimension.x * 4, dimension.y, dimension.z, TextureFormat::R32I);
			}
			else 
			{
				grid.lpvGridR->buildTexture(TextureFormat::R32I,dimension.x * 4, dimension.y, dimension.z);
				grid.lpvGridB->buildTexture(TextureFormat::R32I,dimension.x * 4, dimension.y, dimension.z);
				grid.lpvGridG->buildTexture(TextureFormat::R32I,dimension.x * 4, dimension.y, dimension.z);
			}	
		}
	}

	namespace component
	{
		struct InjectLightData
		{
			std::shared_ptr<Shader> shader;
			std::vector<std::shared_ptr<DescriptorSet>> descriptors;
			BoundingBox boundingBox;
			InjectLightData()
			{
				shader = Shader::create("shaders/LPV/LightInjection.shader");
				descriptors.emplace_back(DescriptorSet::create({0,shader.get()}));
			}
		};
	};

	namespace light_propagation_volume
	{
		namespace inject_light_pass 
		{
			using Entity = ecs::Chain
				::Write<component::LPVGrid>
				::Read<component::ReflectiveShadowData>
				::Read<component::RendererData>
				::Write<component::InjectLightData>
				::Read<component::BoundingBoxComponent>
				::To<ecs::Entity>;
				
			inline auto beginScene(Entity entity, ecs::World world)
			{
				auto [lpv, rsm, render, injectLight,aabb] = entity;
				if (injectLight.boundingBox != *aabb.box) 
				{
					updateGrid(lpv, aabb.box);
					injectLight.boundingBox.min = aabb.box->min;
					injectLight.boundingBox.max = aabb.box->max;
				}

				injectLight.descriptors[0]->setUniform("UniformBufferObject", "minAABB", glm::value_ptr(aabb.box->size()));
				injectLight.descriptors[0]->setUniform("UniformBufferObject", "cellSize", &lpv.cellSize);
			}

			inline auto render(Entity entity, ecs::World world)
			{
				auto [lpv, rsm, rendererData,injectionLight,aabb] = entity;

				if (lpv.lpvGridR == nullptr)
					return;

				lpv.lpvGridR->clear();
				lpv.lpvGridG->clear();
				lpv.lpvGridB->clear();

				injectionLight.descriptors[0]->setTexture("LPVGridR",lpv.lpvGridR);
				injectionLight.descriptors[0]->setTexture("LPVGridG",lpv.lpvGridG);
				injectionLight.descriptors[0]->setTexture("LPVGridB",lpv.lpvGridB);
				injectionLight.descriptors[0]->setTexture("uFluxSampler", rsm.fluxTexture);
				injectionLight.descriptors[0]->setTexture("uRSMWorldSampler", rsm.worldTexture);

				PipelineInfo pipelineInfo;
				pipelineInfo.shader = injectionLight.shader;
				pipelineInfo.groupCountX = component::ReflectiveShadowData::SHADOW_SIZE / injectionLight.shader->getLocalSizeX();
				pipelineInfo.groupCountY = component::ReflectiveShadowData::SHADOW_SIZE / injectionLight.shader->getLocalSizeY();
				auto pipeline = Pipeline::get(pipelineInfo);
				pipeline->bind(rendererData.commandBuffer);
				Renderer::bindDescriptorSets(pipeline.get(), rendererData.commandBuffer, 0, injectionLight.descriptors);
				Renderer::dispatch(rendererData.commandBuffer,pipelineInfo.groupCountX,pipelineInfo.groupCountY,1);
				pipeline->end(rendererData.commandBuffer);
			}
		};

		namespace inject_geometry_pass
		{
			using Entity = ecs::Chain
				::Read<component::LPVGrid>
				::To<ecs::Entity>;

			inline auto beginScene(Entity entity, ecs::World world)
			{

			}

			inline auto render(Entity entity, ecs::World world)
			{

			}
		};

		namespace propagation_pass
		{
			using Entity = ecs::Chain
				::Read<component::LPVGrid>
				::To<ecs::Entity>;

			inline auto beginScene(Entity entity, ecs::World world)
			{

			}

			inline auto render(Entity entity, ecs::World world)
			{

			}
		};


		auto registerLPV(ExecuteQueue& begin, ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerGlobalComponent<component::LPVGrid>();
			executePoint->registerGlobalComponent<component::InjectLightData>();


			executePoint->registerWithinQueue<inject_light_pass::beginScene>(begin);
			executePoint->registerWithinQueue<inject_light_pass::render>(renderer);
			executePoint->registerWithinQueue<inject_geometry_pass::beginScene>(begin);
			executePoint->registerWithinQueue<inject_geometry_pass::render>(renderer);
			executePoint->registerWithinQueue<propagation_pass::beginScene>(begin);
			executePoint->registerWithinQueue<propagation_pass::render>(renderer);
		}
	};
};        // namespace maple
