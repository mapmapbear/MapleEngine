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
			TextureParameters paramemters(TextureFormat::R32I, TextureFilter::Linear, TextureWrap::ClampToEdge);
			if (grid.lpvGridB == nullptr) 
			{

				grid.lpvGridR = Texture3D::create(dimension.x * 4, dimension.y, dimension.z, paramemters);
				grid.lpvGridB = Texture3D::create(dimension.x * 4, dimension.y, dimension.z, paramemters);
				grid.lpvGridG = Texture3D::create(dimension.x * 4, dimension.y, dimension.z, paramemters);
				grid.lpvGeometryVolume = Texture3D::create(dimension.x * 4, dimension.y, dimension.z, paramemters);
				grid.lpvAccumulatorB = Texture3D::create(dimension.x * 4, dimension.y, dimension.z, paramemters);
				grid.lpvAccumulatorG = Texture3D::create(dimension.x * 4, dimension.y, dimension.z, paramemters);
				grid.lpvAccumulatorR = Texture3D::create(dimension.x * 4, dimension.y, dimension.z, paramemters);


				grid.lpvRs.emplace_back(grid.lpvGridR);
				grid.lpvGs.emplace_back(grid.lpvGridG);
				grid.lpvBs.emplace_back(grid.lpvGridB);

				for (auto i = 0;i< grid.propagateCount;i++)
				{
					grid.lpvBs.emplace_back(Texture3D::create(dimension.x * 4, dimension.y, dimension.z, paramemters));
					grid.lpvRs.emplace_back(Texture3D::create(dimension.x * 4, dimension.y, dimension.z, paramemters));
					grid.lpvGs.emplace_back(Texture3D::create(dimension.x * 4, dimension.y, dimension.z, paramemters));
				}
			}
			else 
			{
				grid.lpvGridR->buildTexture3D(TextureFormat::R32I,dimension.x * 4, dimension.y, dimension.z);
				grid.lpvGridB->buildTexture3D(TextureFormat::R32I,dimension.x * 4, dimension.y, dimension.z);
				grid.lpvGridG->buildTexture3D(TextureFormat::R32I,dimension.x * 4, dimension.y, dimension.z);
				grid.lpvGeometryVolume->buildTexture3D(TextureFormat::R32I, dimension.x * 4, dimension.y, dimension.z);
				grid.lpvAccumulatorB->buildTexture3D(TextureFormat::R32I, dimension.x * 4, dimension.y, dimension.z);
				grid.lpvAccumulatorG->buildTexture3D(TextureFormat::R32I, dimension.x * 4, dimension.y, dimension.z);
				grid.lpvAccumulatorR->buildTexture3D(TextureFormat::R32I, dimension.x * 4, dimension.y, dimension.z);

				for (auto i = 1; i <= grid.propagateCount; i++)
				{
					grid.lpvBs[i]->buildTexture3D(TextureFormat::R32I, dimension.x * 4, dimension.y, dimension.z);
					grid.lpvRs[i]->buildTexture3D(TextureFormat::R32I, dimension.x * 4, dimension.y, dimension.z);
					grid.lpvGs[i]->buildTexture3D(TextureFormat::R32I, dimension.x * 4, dimension.y, dimension.z);
				}
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

		struct InjectGeometryVolume
		{
			std::shared_ptr<Shader> shader;
			std::vector<std::shared_ptr<DescriptorSet>> descriptors;
			InjectGeometryVolume()
			{
				shader = Shader::create("shaders/LPV/GeometryInjection.shader");
				descriptors.emplace_back(DescriptorSet::create({ 0,shader.get() }));
			}
		};

		struct PropagationData
		{
			std::shared_ptr<Shader> shader;
			std::vector<std::shared_ptr<DescriptorSet>> descriptors;
			PropagationData()
			{
				shader = Shader::create("shaders/LPV/LightPropagation.shader");
				descriptors.emplace_back(DescriptorSet::create({ 0,shader.get() }));
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

				if (aabb.box == nullptr)
					return;

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
				injectionLight.descriptors[0]->update();

				PipelineInfo pipelineInfo;
				pipelineInfo.shader = injectionLight.shader;
				pipelineInfo.groupCountX = rsm.normalTexture->getWidth() / injectionLight.shader->getLocalSizeX();
				pipelineInfo.groupCountY = rsm.normalTexture->getWidth() / injectionLight.shader->getLocalSizeY();
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
				::Write<component::InjectGeometryVolume>
				::Read<component::BoundingBoxComponent>
				::Read<component::ShadowMapData>
				::Read<component::ReflectiveShadowData>
				::Read<component::RendererData>
				::To<ecs::Entity>;

			inline auto beginScene(Entity entity, ecs::World world)
			{
				auto [lpv, geometry,aabb,shadowData,rsm, rendererData] = entity;
				if (lpv.lpvGridR == nullptr)
					return;
				geometry.descriptors[0]->setUniform("UniformBufferObject", "lightViewMat", glm::value_ptr(shadowData.lightMatrix));
				geometry.descriptors[0]->setUniform("UniformBufferObject", "minAABB", glm::value_ptr(aabb.box->size()));
				geometry.descriptors[0]->setUniform("UniformBufferObject", "lightDir", glm::value_ptr(shadowData.lightDir));
				geometry.descriptors[0]->setUniform("UniformBufferObject", "rsmArea", &shadowData.lightArea);
				geometry.descriptors[0]->setUniform("UniformBufferObject", "cellSize", &lpv.cellSize);
			}

			inline auto render(Entity entity, ecs::World world)
			{
				auto [lpv, geometry, aabb, shadowData, rsm, rendererData] = entity;
				if (lpv.lpvGridR == nullptr)
					return;

				lpv.lpvGeometryVolume->clear();

				geometry.descriptors[0]->setTexture("uGeometryVolume", lpv.lpvGeometryVolume);
				geometry.descriptors[0]->setTexture("uRSMNormalSampler", rsm.normalTexture);
				geometry.descriptors[0]->setTexture("uRSMWorldSampler", rsm.worldTexture);
				geometry.descriptors[0]->update();

				PipelineInfo pipelineInfo;
				pipelineInfo.shader = geometry.shader;
				pipelineInfo.groupCountX = rsm.normalTexture->getWidth() / geometry.shader->getLocalSizeX();
				pipelineInfo.groupCountY = rsm.normalTexture->getHeight() / geometry.shader->getLocalSizeY();
				auto pipeline = Pipeline::get(pipelineInfo);
				pipeline->bind(rendererData.commandBuffer);
				Renderer::bindDescriptorSets(pipeline.get(), rendererData.commandBuffer, 0, geometry.descriptors);
				Renderer::dispatch(rendererData.commandBuffer, pipelineInfo.groupCountX, pipelineInfo.groupCountY, 1);
				pipeline->end(rendererData.commandBuffer);
			}
		};

		namespace propagation_pass
		{
			using Entity = ecs::Chain
				::Read<component::LPVGrid>
				::Write<component::PropagationData>
				::Read<component::BoundingBoxComponent>
				::Write<component::RendererData>
				::To<ecs::Entity>;

			inline auto beginScene(Entity entity, ecs::World world)
			{
				auto [lpv, data, aabb,renderData] = entity;
				if (lpv.lpvGridR == nullptr)
					return;
				data.descriptors[0]->setUniform("UniformObject", "gridDim", glm::value_ptr(aabb.box->size()));
				data.descriptors[0]->setUniform("UniformObject", "occlusionAmplifier", &lpv.occlusionAmplifier);
			}

			inline auto render(Entity entity, ecs::World world)
			{
				auto [lpv, data, aabb, rendererData] = entity;
				if (lpv.lpvGridR == nullptr)
					return;
				data.descriptors[0]->setTexture("uGeometryVolume", lpv.lpvGeometryVolume);

				data.descriptors[0]->setTexture("RAccumulatorLPV_", lpv.lpvAccumulatorR);
				data.descriptors[0]->setTexture("GAccumulatorLPV_", lpv.lpvAccumulatorG);
				data.descriptors[0]->setTexture("BAccumulatorLPV_", lpv.lpvAccumulatorB);

				for (auto i = 1;i<= lpv.propagateCount;i++)
				{
					lpv.lpvBs[i]->clear();
					lpv.lpvRs[i]->clear();
					lpv.lpvGs[i]->clear();
				}

				lpv.lpvAccumulatorR->clear();
				lpv.lpvAccumulatorG->clear();
				lpv.lpvAccumulatorB->clear();

				auto aabbSize = aabb.box->size();
				PipelineInfo pipelineInfo;
				pipelineInfo.shader = data.shader;
				pipelineInfo.groupCountX = aabbSize.x / data.shader->getLocalSizeX();
				pipelineInfo.groupCountY = aabbSize.y / data.shader->getLocalSizeY();
				pipelineInfo.groupCountZ = aabbSize.z / data.shader->getLocalSizeZ();
				auto pipeline = Pipeline::get(pipelineInfo);

				for (auto i = 1; i <= lpv.propagateCount; i++)
				{
					data.descriptors[0]->setTexture("LPVGridR", lpv.lpvRs[i - 1]);
					data.descriptors[0]->setTexture("LPVGridG", lpv.lpvGs[i - 1]);
					data.descriptors[0]->setTexture("LPVGridB", lpv.lpvBs[i - 1]);

					data.descriptors[0]->setTexture("LPVGridR_", lpv.lpvRs[i]);
					data.descriptors[0]->setTexture("LPVGridG_", lpv.lpvGs[i]);
					data.descriptors[0]->setTexture("LPVGridB_", lpv.lpvBs[i]);
					data.descriptors[0]->update();

		
					pipeline->bind(rendererData.commandBuffer);
					Renderer::bindDescriptorSets(pipeline.get(), rendererData.commandBuffer, 0, data.descriptors);
					Renderer::dispatch(rendererData.commandBuffer, pipelineInfo.groupCountX, pipelineInfo.groupCountY, pipelineInfo.groupCountZ);
					pipeline->end(rendererData.commandBuffer);
				}
			}
		};


		auto registerLPV(ExecuteQueue& begin, ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerGlobalComponent<component::LPVGrid>();
			executePoint->registerGlobalComponent<component::InjectLightData>();
			executePoint->registerGlobalComponent<component::InjectGeometryVolume>();
			executePoint->registerGlobalComponent<component::PropagationData>();

			executePoint->registerWithinQueue<inject_light_pass::beginScene>(begin);
			executePoint->registerWithinQueue<inject_light_pass::render>(renderer);
			executePoint->registerWithinQueue<inject_geometry_pass::beginScene>(begin);
			executePoint->registerWithinQueue<inject_geometry_pass::render>(renderer);
			executePoint->registerWithinQueue<propagation_pass::beginScene>(begin);
			executePoint->registerWithinQueue<propagation_pass::render>(renderer);
		}
	};
};        // namespace maple
