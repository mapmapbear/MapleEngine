//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "ReflectiveShadowMap.h"

#include "Engine/Camera.h"
#include "Engine/Core.h"
#include "Engine/Material.h"
#include "Engine/Profiler.h"
#include "Engine/Mesh.h"
#include "Engine/Renderer/RendererData.h"
#include "Engine/CaptureGraph.h"
#include "Engine/Renderer/GeometryRenderer.h"

#include "ImGui/ImGuiHelpers.h"
#include "Math/Frustum.h"
#include "Math/MathUtils.h"

#include "Scene/Component/Light.h"
#include "Scene/Scene.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Component/BoundingBox.h"

#include "RHI/CommandBuffer.h"
#include "RHI/DescriptorSet.h"
#include "RHI/Pipeline.h"
#include "RHI/Shader.h"
#include "RHI/Texture.h"

#include "Application.h"

#include <ecs/ecs.h>

namespace maple
{
	namespace        //private block
	{
		inline auto updateCascades(const component::CameraView & camera, component::ShadowMapData& shadowData, component::Light *light)
		{
			PROFILE_FUNCTION();

			float cascadeSplits[SHADOWMAP_MAX];

			const float nearClip  = camera.nearPlane;
			const float farClip   = camera.farPlane;
			const float clipRange = farClip - nearClip;
			const float minZ      = nearClip;
			const float maxZ      = nearClip + clipRange;
			const float range     = maxZ - minZ;
			const float ratio     = maxZ / minZ;
			// Calculate split depths based on view camera frustum
			// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
			for (uint32_t i = 0; i < shadowData.shadowMapNum; i++)
			{
				float p          = static_cast<float>(i + 1) / static_cast<float>(shadowData.shadowMapNum);
				float log        = minZ * std::pow(ratio, p);
				float uniform    = minZ + range * p;
				float d          = shadowData.cascadeSplitLambda * (log - uniform) + uniform;
				cascadeSplits[i] = (d - nearClip) / clipRange;
			}

			for (uint32_t i = 0; i < shadowData.shadowMapNum; i++)
			{
				PROFILE_SCOPE("Create Cascade");
				float splitDist     = cascadeSplits[i];
				float lastSplitDist = cascadeSplits[i];

				auto frum = camera.frustum;

				glm::vec3 *frustumCorners = frum.getVertices();

				for (uint32_t i = 0; i < 4; i++)
				{
					glm::vec3 dist        = frustumCorners[i + 4] - frustumCorners[i];
					frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
					frustumCorners[i]     = frustumCorners[i] + (dist * lastSplitDist);
				}

				glm::vec3 frustumCenter = glm::vec3(0.0f);
				for (uint32_t i = 0; i < 8; i++)
				{
					frustumCenter += frustumCorners[i];
				}
				frustumCenter /= 8.0f;

				float radius = 0.0f;
				for (uint32_t i = 0; i < 8; i++)
				{
					float distance = glm::length(frustumCorners[i] - frustumCenter);
					radius         = glm::max(radius, distance);
				}
				radius = std::ceil(radius * 16.0f) / 16.0f;

				glm::vec3 maxExtents = glm::vec3(radius);
				glm::vec3 minExtents = -maxExtents;

				glm::vec3 lightDir         = glm::normalize(glm::vec3(light->lightData.direction));
				glm::mat4 lightViewMatrix  = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, maple::UP);
				glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);

				shadowData.splitDepth[i]     = glm::vec4(camera.nearPlane + splitDist * clipRange) * -1.f;
				shadowData.shadowProjView[i] = lightOrthoMatrix * lightViewMatrix;
				if (i == 0) 
				{
					shadowData.lightMatrix = lightViewMatrix;
					shadowData.lightDir = lightDir;
				}
			}
		}
	}        // namespace

	component::ShadowMapData::ShadowMapData()
	{
		shadowTexture = TextureDepthArray::create(SHADOWMAP_SiZE_MAX, SHADOWMAP_SiZE_MAX, shadowMapNum);
		shader        = Shader::create("shaders/Shadow.shader");

		DescriptorInfo createInfo{};
		createInfo.layoutIndex = 0;
		createInfo.shader      = shader.get();

		descriptorSet.resize(1);
		descriptorSet[0] = DescriptorSet::create(createInfo);
		currentDescriptorSets.resize(1);
		cascadeCommandQueue[0].reserve(500);
		cascadeCommandQueue[1].reserve(500);
		cascadeCommandQueue[2].reserve(500);
		cascadeCommandQueue[3].reserve(500);

		shadowTexture->setName("uShaderMapSampler");
	}

	component::ReflectiveShadowData::ReflectiveShadowData()
	{
		shader = Shader::create("shaders/LPV/ReflectiveShadowMap.shader");
		descriptorSets.resize(2);
		descriptorSets[0] = DescriptorSet::create({ 0, shader.get() });
		descriptorSets[1] = DescriptorSet::create({ 1, shader.get() });

		TextureParameters parameters;

		parameters.format = TextureFormat::RGBA32;
		parameters.wrap = TextureWrap::ClampToBorder;

		fluxTexture = Texture2D::create(SHADOW_SIZE, SHADOW_SIZE, nullptr, parameters);
		fluxTexture->setName("uFluxSampler");

		worldTexture = Texture2D::create(SHADOW_SIZE, SHADOW_SIZE, nullptr, parameters);
		worldTexture->setName("uRSMWorldSampler");

		normalTexture = Texture2D::create(SHADOW_SIZE, SHADOW_SIZE, nullptr, parameters);
		normalTexture->setName("uRSMNormalSampler");

		fluxDepth = TextureDepth::create(SHADOW_SIZE, SHADOW_SIZE);
		fluxDepth->setName("uFluxDepthSampler");
	}

	namespace shadow_map_pass
	{
		using Entity = ecs::Chain
			::Write<component::ShadowMapData>
			::Read<component::CameraView>
			::Write<component::ReflectiveShadowData>
			::To<ecs::Entity>;

		using LightQuery = ecs::Chain
			::Write<component::Light>
			::To<ecs::Query>;

		using MeshQuery = ecs::Chain
			::Write<component::MeshRenderer>
			::Write<component::Transform>
			::To<ecs::Query>;

		using MeshEntity = ecs::Chain
			::Write<component::MeshRenderer>
			::Write<component::Transform>
			::To<ecs::Entity>;

		auto beginScene(Entity entity, LightQuery lightQuery, MeshQuery meshQuery, ecs::World world)
		{
			auto [shadowData,cameraView,rsm] = entity;

			for (uint32_t i = 0; i < shadowData.shadowMapNum; i++)
			{
				shadowData.cascadeCommandQueue[i].clear();
			}

			if (!lightQuery.empty())
			{
				component::Light* directionaLight = nullptr;

				for (auto entity : lightQuery)
				{
					auto [light] = lightQuery.convert(entity);
					if (static_cast<component::LightType>(light.lightData.type) == component::LightType::DirectionalLight)
					{
						directionaLight = &light;
						break;
					}
				}

				if (directionaLight && directionaLight->castShadow)
				{
					rsm.descriptorSets[1]->setUniform("UBO", "light", &directionaLight->lightData);

					if (directionaLight)
					{
						updateCascades(cameraView, shadowData,directionaLight);

						for (uint32_t i = 0; i < shadowData.shadowMapNum; i++)
						{
							shadowData.cascadeFrustums[i].from(shadowData.shadowProjView[i]);
						}
					}
#pragma omp parallel for num_threads(4)
						for (int32_t i = 0; i < shadowData.shadowMapNum; i++)
						{
							meshQuery.forEach([&, i](MeshEntity meshEntity) {
								auto [mesh, trans] = meshEntity;
								if (mesh.castShadow) 
								{
									auto bb = mesh.getMesh()->getBoundingBox()->transform(trans.getWorldMatrix());
									auto inside = shadowData.cascadeFrustums[i].isInside(bb);
									if (inside)
									{
										auto& cmd = shadowData.cascadeCommandQueue[i].emplace_back();
										cmd.mesh = mesh.getMesh().get();
										cmd.transform = trans.getWorldMatrix();

										if (mesh.getMesh()->getSubMeshCount() <= 1) // at least two subMeshes.
										{
											cmd.material = !mesh.getMesh()->getMaterial().empty() ? mesh.getMesh()->getMaterial()[0].get() : nullptr;
										}
									}
								}});
						}

					shadowData.descriptorSet[0]->setUniform("UniformBufferObject", "projView", shadowData.shadowProjView);
				}
			}
		}

		using RenderEntity = ecs::Chain
			::Write<component::ShadowMapData>
			::Read<component::RendererData>
			::Write<capture_graph::component::RenderGraph>
			::Read<component::ReflectiveShadowData>
			::To<ecs::Entity>;

		inline auto onRender(RenderEntity entity, ecs::World world)
		{
			auto [shadowData, rendererData,renderGraph,rsm] = entity;


			shadowData.descriptorSet[0]->update();

			PipelineInfo pipelineInfo;
			pipelineInfo.shader = shadowData.shader;

			pipelineInfo.cullMode = CullMode::Back;
			pipelineInfo.transparencyEnabled = false;
			pipelineInfo.depthBiasEnabled = false;
			pipelineInfo.depthArrayTarget = shadowData.shadowTexture;
			pipelineInfo.clearTargets = true;
			
			auto pipeline = Pipeline::get(pipelineInfo, shadowData.descriptorSet, renderGraph);

			for (uint32_t i = 0; i < shadowData.shadowMapNum; ++i)
			{
				//GPUProfile("Shadow Layer Pass");
				pipeline->bind(rendererData.commandBuffer, i);

				for (auto& command : shadowData.cascadeCommandQueue[i])
				{
					Mesh* mesh = command.mesh;
					shadowData.currentDescriptorSets[0] = shadowData.descriptorSet[0];
					auto  trans = command.transform;
					auto& pushConstants = shadowData.shader->getPushConstants()[0];

					pushConstants.setValue("transform", (void*)&trans);
					pushConstants.setValue("cascadeIndex", (void*)&i);

					shadowData.shader->bindPushConstants(rendererData.commandBuffer, pipeline.get());

					Renderer::bindDescriptorSets(pipeline.get(), rendererData.commandBuffer, 0, shadowData.descriptorSet);
					Renderer::drawMesh(rendererData.commandBuffer, pipeline.get(), mesh);
				}
				pipeline->end(rendererData.commandBuffer);
			}

		}
	}

	namespace reflective_shadow_map_pass
	{
		namespace begin_scene 
		{
			using Entity = ecs::Chain
				::Read<component::CameraView>
				::Write<component::ReflectiveShadowData>
				::Read<component::BoundingBoxComponent>
				::To<ecs::Entity>;

			using LightQuery = ecs::Chain
				::Write<component::Light>
				::To<ecs::Query>;

			using MeshQuery = ecs::Chain
				::Write<component::MeshRenderer>
				::Write<component::Transform>
				::To<ecs::Query>;

			using MeshEntity = ecs::Chain
				::Write<component::MeshRenderer>
				::Write<component::Transform>
				::To<ecs::Entity>;

			inline auto system(Entity entity, LightQuery lightQuery, MeshQuery meshQuery, ecs::World world)
			{
				auto [cameraView, rsm,aabb] = entity;
				rsm.commandQueue.clear();

				if (!aabb.box ) 
				{
					return;
				}
				
				if (!lightQuery.empty())
				{
					component::Light* directionaLight = nullptr;

					for (auto entity : lightQuery)
					{
						auto [light] = lightQuery.convert(entity);
						if (static_cast<component::LightType>(light.lightData.type) == component::LightType::DirectionalLight)
						{
							directionaLight = &light;
							break;
						}
					}

					if (directionaLight)
					{
						rsm.descriptorSets[1]->setUniform("UBO", "light", &directionaLight->lightData);

						if (directionaLight)
						{
							float distance = glm::length(aabb.box->max - aabb.box->min) / 2;

							glm::vec3 maxExtents = glm::vec3(distance);
							glm::vec3 minExtents = -maxExtents;

							glm::vec3 lightDir = glm::normalize(glm::vec3(directionaLight->lightData.direction) );
							glm::mat4 lightViewMatrix = glm::lookAt(aabb.box->center() + lightDir * minExtents.z, aabb.box->center(), maple::UP);
							const auto len = (maxExtents.z - minExtents.z);

							auto lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, -len, len);

							rsm.projView = lightOrthoMatrix * lightViewMatrix;
							rsm.frustum.from(rsm.projView);
							rsm.lightMatrix = lightViewMatrix;
							rsm.lightArea = distance * distance * 4;
						}
					
						for (auto meshEntity : meshQuery)
						{
							auto [mesh, trans] = meshQuery.convert(meshEntity);

							auto inside = rsm.frustum.isInside(mesh.getMesh()->getBoundingBox()->transform(trans.getWorldMatrix()));

							if (inside)
							{
								auto& cmd = rsm.commandQueue.emplace_back();
								cmd.mesh = mesh.getMesh().get();
								cmd.transform = trans.getWorldMatrix();

								if (mesh.getMesh()->getSubMeshCount() <= 1) // at least two subMeshes.
								{
									cmd.material = !mesh.getMesh()->getMaterial().empty() ? mesh.getMesh()->getMaterial()[0].get() : nullptr;
								}
							}
						}
					}
				}
			}
		}

		using Entity = ecs::Chain
			::Read<component::ReflectiveShadowData>
			::Read<component::RendererData>
			::Write<capture_graph::component::RenderGraph>
			::To<ecs::Entity>;

		inline auto onRender(Entity entity, ecs::World world)
		{
			auto [rsm,renderData,renderGraph] = entity;

			auto descriptorSet = rsm.descriptorSets[1];

			rsm.descriptorSets[0]->setUniform("UniformBufferObject", "lightProjection", &rsm.projView);

			rsm.descriptorSets[0]->update();
			rsm.descriptorSets[1]->update();

			auto commandBuffer = renderData.commandBuffer;

			PipelineInfo pipeInfo;
			pipeInfo.shader = rsm.shader;
			pipeInfo.transparencyEnabled = false;
			pipeInfo.depthBiasEnabled = false;
			pipeInfo.clearTargets = true;
			pipeInfo.depthTarget = rsm.fluxDepth;
			pipeInfo.colorTargets[0] = rsm.fluxTexture;
			pipeInfo.colorTargets[1] = rsm.worldTexture;
			pipeInfo.colorTargets[2] = rsm.normalTexture;
			pipeInfo.clearColor = { 0, 0, 0, 0 };

			auto pipeline = Pipeline::get(pipeInfo, rsm.descriptorSets, renderGraph);

			if (commandBuffer)
				commandBuffer->bindPipeline(pipeline.get());
			else
				pipeline->bind(commandBuffer);

			for (auto& command : rsm.commandQueue)
			{
				Mesh* mesh = command.mesh;
				const auto& trans = command.transform;
				auto& pushConstants = rsm.shader->getPushConstants()[0];

				pushConstants.setValue("transform", (void*)&trans);

				rsm.shader->bindPushConstants(commandBuffer, pipeline.get());
			
				if (mesh->getSubMeshCount() > 1)
				{
					auto& materials = mesh->getMaterial();
					auto& indices = mesh->getSubMeshIndex();
					auto start = 0;
					mesh->getVertexBuffer()->bind(commandBuffer, pipeline.get());
					mesh->getIndexBuffer()->bind(commandBuffer);
					for (auto i = 0; i <= indices.size(); i++)
					{
						auto & material = materials[i];
						auto end = i == indices.size() ? command.mesh->getIndexBuffer()->getCount() : indices[i];

						descriptorSet->setUniform("UBO", "albedoColor", &material->getProperties().albedoColor);
						descriptorSet->setUniform("UBO", "usingAlbedoMap", &material->getProperties().usingAlbedoMap);
						descriptorSet->setTexture("uDiffuseMap", material->getTextures().albedo);
						descriptorSet->update();

						Renderer::bindDescriptorSets(pipeline.get(), commandBuffer, 0, rsm.descriptorSets);
						Renderer::drawIndexed(commandBuffer, DrawType::Triangle, end - start, start);

						start = end;
					}
					mesh->getVertexBuffer()->unbind();
					mesh->getIndexBuffer()->unbind();
				}
				else 
				{
					if (command.material != nullptr)
					{
						descriptorSet->setUniform("UBO", "albedoColor", &command.material->getProperties().albedoColor);
						descriptorSet->setUniform("UBO", "usingAlbedoMap", &command.material->getProperties().usingAlbedoMap);
						descriptorSet->setTexture("uDiffuseMap", command.material->getTextures().albedo);
						descriptorSet->update();
					}
					Renderer::bindDescriptorSets(pipeline.get(), commandBuffer, 0, rsm.descriptorSets);
					Renderer::drawMesh(commandBuffer, pipeline.get(), mesh);
				}
			}

			if (commandBuffer)
				commandBuffer->unbindPipeline();
			else
				pipeline->end(commandBuffer);
		}
	}

	namespace reflective_shadow_map
	{
		auto registerShadowMap(ExecuteQueue& begin, ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerGlobalComponent<component::ShadowMapData>();
			executePoint->registerGlobalComponent<component::ReflectiveShadowData>();

			executePoint->registerWithinQueue<shadow_map_pass::beginScene>(begin);
			executePoint->registerWithinQueue<shadow_map_pass::onRender>(renderer);

			executePoint->registerWithinQueue<reflective_shadow_map_pass::begin_scene::system>(begin);
			executePoint->registerWithinQueue<reflective_shadow_map_pass::onRender>(renderer);
		}
	};
};        // namespace maple
