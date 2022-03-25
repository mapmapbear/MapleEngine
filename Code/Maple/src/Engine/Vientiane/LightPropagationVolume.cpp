//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "LightPropagationVolume.h"
#include "ReflectiveShadowMap.h"
#include "Scene/Component/BoundingBox.h"
#include "Math/BoundingBox.h"

#include "Engine/Renderer/RendererData.h"
#include "Engine/Mesh.h"
#include "Engine/GBuffer.h"

#include "RHI/CommandBuffer.h"
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
			TextureParameters paramemters(TextureFormat::R32UI, TextureFilter::Nearest, TextureWrap::ClampToEdge);
			if (grid.lpvGridB == nullptr) 
			{

				grid.lpvGridR = Texture3D::create(grid.gridDimension.x * 4, grid.gridDimension.y, grid.gridDimension.z, paramemters);
				grid.lpvGridB = Texture3D::create(grid.gridDimension.x * 4, grid.gridDimension.y, grid.gridDimension.z, paramemters);
				grid.lpvGridG = Texture3D::create(grid.gridDimension.x * 4, grid.gridDimension.y, grid.gridDimension.z, paramemters);
				
				grid.lpvGeometryVolumeR = Texture3D::create(grid.gridDimension.x * 4, grid.gridDimension.y, grid.gridDimension.z, paramemters);
				grid.lpvGeometryVolumeG = Texture3D::create(grid.gridDimension.x * 4, grid.gridDimension.y, grid.gridDimension.z, paramemters);
				grid.lpvGeometryVolumeB = Texture3D::create(grid.gridDimension.x * 4, grid.gridDimension.y, grid.gridDimension.z, paramemters);

				grid.lpvAccumulatorB = Texture3D::create(grid.gridDimension.x * 4, grid.gridDimension.y, grid.gridDimension.z, paramemters);
				grid.lpvAccumulatorG = Texture3D::create(grid.gridDimension.x * 4, grid.gridDimension.y, grid.gridDimension.z, paramemters);
				grid.lpvAccumulatorR = Texture3D::create(grid.gridDimension.x * 4, grid.gridDimension.y, grid.gridDimension.z, paramemters);


				grid.lpvRs.emplace_back(grid.lpvGridR);
				grid.lpvGs.emplace_back(grid.lpvGridG);
				grid.lpvBs.emplace_back(grid.lpvGridB);

				for (auto i = 0;i< grid.propagateCount;i++)
				{
					grid.lpvBs.emplace_back(Texture3D::create(grid.gridDimension.x * 4, grid.gridDimension.y, grid.gridDimension.z, paramemters));
					grid.lpvRs.emplace_back(Texture3D::create(grid.gridDimension.x * 4, grid.gridDimension.y, grid.gridDimension.z, paramemters));
					grid.lpvGs.emplace_back(Texture3D::create(grid.gridDimension.x * 4, grid.gridDimension.y, grid.gridDimension.z, paramemters));
				}
			}
			else 
			{
				grid.lpvGridR->buildTexture3D(TextureFormat::R32UI,grid.gridDimension.x * 4, grid.gridDimension.y, grid.gridDimension.z);
				grid.lpvGridB->buildTexture3D(TextureFormat::R32UI,grid.gridDimension.x * 4, grid.gridDimension.y, grid.gridDimension.z);
				grid.lpvGridG->buildTexture3D(TextureFormat::R32UI,grid.gridDimension.x * 4, grid.gridDimension.y, grid.gridDimension.z);

				grid.lpvGeometryVolumeR->buildTexture3D(TextureFormat::R32UI, grid.gridDimension.x * 4, grid.gridDimension.y, grid.gridDimension.z);
				grid.lpvGeometryVolumeG->buildTexture3D(TextureFormat::R32UI, grid.gridDimension.x * 4, grid.gridDimension.y, grid.gridDimension.z);
				grid.lpvGeometryVolumeB->buildTexture3D(TextureFormat::R32UI, grid.gridDimension.x * 4, grid.gridDimension.y, grid.gridDimension.z);

				grid.lpvAccumulatorB->buildTexture3D(TextureFormat::R32UI, grid.gridDimension.x * 4, grid.gridDimension.y, grid.gridDimension.z);
				grid.lpvAccumulatorG->buildTexture3D(TextureFormat::R32UI, grid.gridDimension.x * 4, grid.gridDimension.y, grid.gridDimension.z);
				grid.lpvAccumulatorR->buildTexture3D(TextureFormat::R32UI, grid.gridDimension.x * 4, grid.gridDimension.y, grid.gridDimension.z);

				for (auto i = 1; i <= grid.propagateCount; i++)
				{
					grid.lpvBs[i]->buildTexture3D(TextureFormat::R32UI, grid.gridDimension.x * 4, grid.gridDimension.y, grid.gridDimension.z);
					grid.lpvRs[i]->buildTexture3D(TextureFormat::R32UI, grid.gridDimension.x * 4, grid.gridDimension.y, grid.gridDimension.z);
					grid.lpvGs[i]->buildTexture3D(TextureFormat::R32UI, grid.gridDimension.x * 4, grid.gridDimension.y, grid.gridDimension.z);
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

		struct DebugAABBData
		{
			std::shared_ptr<Shader> shader;
			std::vector<std::shared_ptr<DescriptorSet>> descriptors;
			std::shared_ptr<Mesh> sphere;
			DebugAABBData()
			{
				shader = Shader::create("shaders/LPV/AABBDebug.shader");
				descriptors.emplace_back(DescriptorSet::create({ 0,shader.get() }));
				descriptors.emplace_back(DescriptorSet::create({ 1,shader.get() }));
				sphere = Mesh::createSphere();
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
					auto size = aabb.box->size();
					auto maxValue = std::max(size.x, std::max(size.y, size.z));
					lpv.cellSize = maxValue / lpv.gridSize;
					lpv.gridDimension = size / lpv.cellSize;
					glm::ceil(lpv.gridDimension);
					lpv.gridDimension = glm::min(lpv.gridDimension, {32,32,32});

					updateGrid(lpv, aabb.box);

					injectLight.boundingBox.min = aabb.box->min;
					injectLight.boundingBox.max = aabb.box->max;

					injectLight.descriptors[0]->setUniform("UniformBufferObject", "gridSize", &lpv.gridSize);
					injectLight.descriptors[0]->setUniform("UniformBufferObject", "minAABB", glm::value_ptr(injectLight.boundingBox.min));
					injectLight.descriptors[0]->setUniform("UniformBufferObject", "cellSize", &lpv.cellSize);
				}
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
				pipelineInfo.groupCountY = rsm.normalTexture->getHeight() / injectionLight.shader->getLocalSizeY();
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
				geometry.descriptors[0]->setUniform("UniformBufferObject", "lightViewMat", glm::value_ptr(rsm.lightMatrix));
				geometry.descriptors[0]->setUniform("UniformBufferObject", "minAABB", glm::value_ptr(aabb.box->min));
				geometry.descriptors[0]->setUniform("UniformBufferObject", "cellSize", &lpv.cellSize);
				geometry.descriptors[0]->setUniform("UniformBufferObject", "lightDir", glm::value_ptr(shadowData.lightDir));
				geometry.descriptors[0]->setUniform("UniformBufferObject", "rsmArea", &rsm.lightArea);
			}

			inline auto render(Entity entity, ecs::World world)
			{
				auto [lpv, geometry, aabb, shadowData, rsm, rendererData] = entity;
				if (lpv.lpvGridR == nullptr)
					return;

				lpv.lpvGeometryVolumeR->clear();
				lpv.lpvGeometryVolumeG->clear();
				lpv.lpvGeometryVolumeB->clear();

				geometry.descriptors[0]->setTexture("uGeometryVolumeR", lpv.lpvGeometryVolumeR);
				geometry.descriptors[0]->setTexture("uGeometryVolumeG", lpv.lpvGeometryVolumeG);
				geometry.descriptors[0]->setTexture("uGeometryVolumeB", lpv.lpvGeometryVolumeB);
				geometry.descriptors[0]->setTexture("uRSMNormalSampler", rsm.normalTexture);
				geometry.descriptors[0]->setTexture("uRSMWorldSampler", rsm.worldTexture);
				geometry.descriptors[0]->setTexture("uFluxSampler", rsm.fluxTexture);

				geometry.descriptors[0]->update();

				PipelineInfo pipelineInfo;
				pipelineInfo.shader = geometry.shader;
				pipelineInfo.groupCountX = rsm.normalTexture->getWidth() / geometry.shader->getLocalSizeX();
				pipelineInfo.groupCountY = rsm.normalTexture->getHeight() / geometry.shader->getLocalSizeY();
				auto pipeline = Pipeline::get(pipelineInfo);
				pipeline->bind(rendererData.commandBuffer);
				Renderer::bindDescriptorSets(pipeline.get(), rendererData.commandBuffer, 0, geometry.descriptors);
				Renderer::dispatch(rendererData.commandBuffer, pipelineInfo.groupCountX, pipelineInfo.groupCountY, 1);
				Renderer::memoryBarrier(rendererData.commandBuffer, MemoryBarrierFlags::Shader_Image_Access_Barrier);
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
				if (lpv.lpvGridR == nullptr )
					return;
				data.descriptors[0]->setTexture("uGeometryVolumeR", lpv.lpvGeometryVolumeR);
				data.descriptors[0]->setTexture("uGeometryVolumeG", lpv.lpvGeometryVolumeG);
				data.descriptors[0]->setTexture("uGeometryVolumeB", lpv.lpvGeometryVolumeB);

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

				PipelineInfo pipelineInfo;
				pipelineInfo.shader = data.shader;
				pipelineInfo.groupCountX = 32 / data.shader->getLocalSizeX();
				pipelineInfo.groupCountY = 32 / data.shader->getLocalSizeY();
				pipelineInfo.groupCountZ = 32 / data.shader->getLocalSizeZ();
				auto pipeline = Pipeline::get(pipelineInfo);
				pipeline->bind(rendererData.commandBuffer);
				for (auto i = 1; i <= lpv.propagateCount; i++)
				{
					data.descriptors[0]->setTexture("LPVGridR", lpv.lpvRs[i - 1]);
					data.descriptors[0]->setTexture("LPVGridG", lpv.lpvGs[i - 1]);
					data.descriptors[0]->setTexture("LPVGridB", lpv.lpvBs[i - 1]);

					data.descriptors[0]->setTexture("LPVGridR_", lpv.lpvRs[i]);
					data.descriptors[0]->setTexture("LPVGridG_", lpv.lpvGs[i]);
					data.descriptors[0]->setTexture("LPVGridB_", lpv.lpvBs[i]);  
					data.descriptors[0]->setUniform("UniformObject", "step", &i );
					data.descriptors[0]->update();
					Renderer::bindDescriptorSets(pipeline.get(), rendererData.commandBuffer, 0, data.descriptors);
					Renderer::dispatch(rendererData.commandBuffer, pipelineInfo.groupCountX, pipelineInfo.groupCountY, pipelineInfo.groupCountZ);
				}
				pipeline->end(rendererData.commandBuffer);
			}
		};
		
		namespace aabb_debug 
		{
			using Entity = ecs::Chain
				::Read<component::LPVGrid>
				::Write<component::DebugAABBData>
				::Read<component::BoundingBoxComponent>
				::Write<component::RendererData>
				::Read<component::CameraView>
				::To<ecs::Entity>;

			inline auto beginScene(Entity entity, ecs::World world)
			{
				auto [lpv, data, aabb, renderData,cameraView] = entity;
				if (lpv.lpvGridR == nullptr || !lpv.debugAABB)
					return;

				data.descriptors[0]->setUniform("UniformBufferObjectVert", "projView", glm::value_ptr(cameraView.projView));

				data.descriptors[1]->setUniform("UniformBufferObjectFrag", "minAABB", glm::value_ptr(aabb.box->min));
				data.descriptors[1]->setUniform("UniformBufferObjectFrag", "cellSize", &lpv.cellSize);
			}

			inline auto render(Entity entity, ecs::World world)
			{
				auto [lpv, data, aabb, renderData, cameraView] = entity;
				if (lpv.lpvGridR == nullptr || !lpv.debugAABB)
					return;

				if (lpv.showGeometry) 
				{
					data.descriptors[1]->setTexture("uRAccumulatorLPV", lpv.lpvGeometryVolumeR);
					data.descriptors[1]->setTexture("uGAccumulatorLPV", lpv.lpvGeometryVolumeG);
					data.descriptors[1]->setTexture("uBAccumulatorLPV", lpv.lpvGeometryVolumeB);
				}
				else 
				{
					data.descriptors[1]->setTexture("uRAccumulatorLPV", lpv.lpvAccumulatorR);
					data.descriptors[1]->setTexture("uGAccumulatorLPV", lpv.lpvAccumulatorG);
					data.descriptors[1]->setTexture("uBAccumulatorLPV", lpv.lpvAccumulatorB);
				}

				for (auto descriptor : data.descriptors)
				{
					descriptor->update();
				}

				auto min = aabb.box->min;
				auto max = aabb.box->max;

				PipelineInfo pipelineInfo{};
				pipelineInfo.shader = data.shader;
				pipelineInfo.polygonMode = PolygonMode::Fill;
				pipelineInfo.blendMode = BlendMode::SrcAlphaOneMinusSrcAlpha;
				pipelineInfo.clearTargets = false;
				pipelineInfo.swapChainTarget = false;
				pipelineInfo.depthTarget = renderData.gbuffer->getDepthBuffer();
				pipelineInfo.colorTargets[0] = renderData.gbuffer->getBuffer(GBufferTextures::SCREEN);

				auto pipeline = Pipeline::get(pipelineInfo);

				if (renderData.commandBuffer)
					renderData.commandBuffer->bindPipeline(pipeline.get());
				else
					pipeline->bind(renderData.commandBuffer);

				const auto r = 0.1 * lpv.cellSize;

				for (float i = min.x; i < max.x; i += lpv.cellSize)
				{
					for (float j = min.y; j < max.y; j += lpv.cellSize)
					{
						for (float k = min.z; k < max.z; k += lpv.cellSize)
						{
							glm::mat4 model = glm::mat4(1);
							model = glm::translate(model, glm::vec3(i, j, k));
							model = glm::scale(model, glm::vec3(r,r,r));

							auto& pushConstants = data.shader->getPushConstants()[0];
							pushConstants.setValue("transform", &model);
							data.shader->bindPushConstants(renderData.commandBuffer, pipeline.get());

							Renderer::bindDescriptorSets(pipeline.get(), renderData.commandBuffer, 0, data.descriptors);
							Renderer::drawMesh(renderData.commandBuffer, pipeline.get(), data.sphere.get());
						}
					}
				}

				if (renderData.commandBuffer)
					renderData.commandBuffer->unbindPipeline();
				else if (pipeline)
					pipeline->end(renderData.commandBuffer);
			}
		}

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

		auto registerLPVDebug(ExecuteQueue& begin, ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerGlobalComponent<component::DebugAABBData>();
			executePoint->registerWithinQueue<aabb_debug::beginScene>(begin);
			executePoint->registerWithinQueue<aabb_debug::render>(renderer);
		}
	};
};        // namespace maple
