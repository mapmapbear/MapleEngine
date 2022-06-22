//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "ReflectiveShadowMap.h"

#include "Engine/Camera.h"
#include "Engine/CaptureGraph.h"
#include "Engine/Core.h"
#include "Engine/Material.h"
#include "Engine/Mesh.h"
#include "Engine/Profiler.h"
#include "Engine/Renderer/GeometryRenderer.h"
#include "Engine/Renderer/RendererData.h"

#include "ImGui/ImGuiHelpers.h"
#include "Math/Frustum.h"
#include "Math/MathUtils.h"

#include "Scene/Component/BoundingBox.h"
#include "Scene/Component/Light.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Scene.h"

#include "RHI/CommandBuffer.h"
#include "RHI/DescriptorSet.h"
#include "RHI/Pipeline.h"
#include "RHI/Shader.h"
#include "RHI/Texture.h"

#include "Application.h"

#include <ecs/ecs.h>

namespace maple
{
	namespace reflective_shadow_map_pass
	{
		namespace begin_scene
		{
			using Entity = ecs::Registry ::Fetch<component::CameraView>::Modify<component::ReflectiveShadowData>::Fetch<component::BoundingBoxComponent>::To<ecs::Entity>;

			using LightQuery = ecs::Registry ::Modify<component::Light>::To<ecs::Group>;

			using MeshQuery = ecs::Registry ::Modify<component::MeshRenderer>::Modify<component::Transform>::To<ecs::Group>;

			using MeshEntity = ecs::Registry ::Modify<component::MeshRenderer>::Modify<component::Transform>::To<ecs::Entity>;

			inline auto system(Entity                         entity,
			                   LightQuery                     lightQuery,
			                   MeshQuery                      meshQuery,
			                   const component::RendererData &renderData,
			                   ecs::World                     world)
			{
				auto [cameraView, rsm, aabb] = entity;
				rsm.commandQueue.clear();

				if (!aabb.box)
				{
					return;
				}

				if (!lightQuery.empty())
				{
					component::Light *directionaLight = nullptr;

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
						rsm.descriptorSets[2]->setUniform("LightUBO", "light", &directionaLight->lightData);
						rsm.descriptorSets[2]->update(renderData.commandBuffer);

						if (directionaLight)
						{
							float distance = glm::length(aabb.box->max - aabb.box->min) / 2;

							glm::vec3 maxExtents = glm::vec3(distance);
							glm::vec3 minExtents = -maxExtents;

							glm::vec3  lightDir        = glm::normalize(glm::vec3(directionaLight->lightData.direction));
							glm::mat4  lightViewMatrix = glm::lookAt(aabb.box->center() + lightDir * minExtents.z, aabb.box->center(), maple::UP);
							const auto len             = (maxExtents.z - minExtents.z);

							auto lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, -len, len);

							rsm.projView = lightOrthoMatrix * lightViewMatrix;
							rsm.frustum.from(rsm.projView);
							rsm.lightMatrix = lightViewMatrix;
							rsm.lightArea   = distance * distance * 4;
						}

						for (auto meshEntity : meshQuery)
						{
							auto [mesh, trans] = meshQuery.convert(meshEntity);

							if (mesh.active && mesh.mesh)
							{
								auto inside = rsm.frustum.isInside(mesh.mesh->getBoundingBox()->transform(trans.getWorldMatrix()));
								if (inside)
								{
									auto &cmd     = rsm.commandQueue.emplace_back();
									cmd.mesh      = mesh.mesh.get();
									cmd.transform = trans.getWorldMatrix();

									for (auto material : mesh.mesh->getMaterial())
									{
										material->setShader(rsm.shader, true);
									}
								}
							}
						}
					}
				}
			}
		}        // namespace begin_scene

		using Entity = ecs::Registry ::Modify<component::ReflectiveShadowData>::Fetch<component::RendererData>::Modify<capture_graph::component::RenderGraph>::To<ecs::Entity>;

		inline auto onRender(Entity entity, ecs::World world)
		{
			auto [rsm, renderData, renderGraph] = entity;

			rsm.descriptorSets[0]->setUniform("UniformBufferObject", "lightProjection", &rsm.projView);

			auto commandBuffer = renderData.commandBuffer;

			rsm.descriptorSets[0]->update(commandBuffer);

			PipelineInfo pipeInfo;
			pipeInfo.shader              = rsm.shader;
			pipeInfo.transparencyEnabled = false;
			pipeInfo.depthBiasEnabled    = false;
			pipeInfo.clearTargets        = true;
			pipeInfo.depthTarget         = rsm.fluxDepth;
			pipeInfo.colorTargets[0]     = rsm.fluxTexture;
			pipeInfo.colorTargets[1]     = rsm.worldTexture;
			pipeInfo.colorTargets[2]     = rsm.normalTexture;
			pipeInfo.clearColor          = {0, 0, 0, 0};

			auto pipeline = Pipeline::get(pipeInfo, rsm.descriptorSets, renderGraph);

			if (commandBuffer)
				commandBuffer->bindPipeline(pipeline.get());
			else
				pipeline->bind(commandBuffer);

			for (auto &command : rsm.commandQueue)
			{
				Mesh *      mesh          = command.mesh;
				const auto &trans         = command.transform;
				auto &      pushConstants = rsm.shader->getPushConstants()[0];

				pushConstants.setValue("transform", (void *) &trans);

				rsm.shader->bindPushConstants(commandBuffer, pipeline.get());

				auto &materials = mesh->getMaterial();
				auto &indices   = mesh->getSubMeshIndex();
				auto  start     = 0;
				mesh->getVertexBuffer()->bind(commandBuffer, pipeline.get());
				mesh->getIndexBuffer()->bind(commandBuffer);
				for (auto i = 0; i < indices.size(); i++)
				{
					auto material = indices.size() > materials.size() ? rsm.defaultMaterial : materials[i];
					auto end      = indices[i];
					material->bind(renderData.commandBuffer);
					rsm.descriptorSets[1] = material->getDescriptorSet();
					Renderer::bindDescriptorSets(pipeline.get(), commandBuffer, 0, rsm.descriptorSets);
					Renderer::drawIndexed(commandBuffer, DrawType::Triangle, end - start, start);
					start = end;
				}
				mesh->getVertexBuffer()->unbind();
				mesh->getIndexBuffer()->unbind();
			}

			if (commandBuffer)
				commandBuffer->unbindPipeline();
			else
				pipeline->end(commandBuffer);
		}
	}        // namespace reflective_shadow_map_pass

	namespace reflective_shadow_map
	{
		auto registerGlobalComponent(std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerGlobalComponent<component::ReflectiveShadowData>([](component::ReflectiveShadowData &data) {
				data.shader = Shader::create("shaders/LPV/ReflectiveShadowMap.shader");
				data.descriptorSets.resize(3);
				data.descriptorSets[0] = DescriptorSet::create({0, data.shader.get()});
				data.descriptorSets[2] = DescriptorSet::create({2, data.shader.get()});
				TextureParameters parameters;

				parameters.format = TextureFormat::RGBA32;
				parameters.wrap   = TextureWrap::ClampToBorder;

				data.fluxTexture = Texture2D::create(component::ReflectiveShadowData::SHADOW_SIZE, component::ReflectiveShadowData::SHADOW_SIZE, nullptr, parameters);
				data.fluxTexture->setName("uFluxSampler");

				data.worldTexture = Texture2D::create(component::ReflectiveShadowData::SHADOW_SIZE, component::ReflectiveShadowData::SHADOW_SIZE, nullptr, parameters);
				data.worldTexture->setName("uRSMWorldSampler");

				data.normalTexture = Texture2D::create(component::ReflectiveShadowData::SHADOW_SIZE, component::ReflectiveShadowData::SHADOW_SIZE, nullptr, parameters);
				data.normalTexture->setName("uRSMNormalSampler");

				data.fluxDepth = TextureDepth::create(component::ReflectiveShadowData::SHADOW_SIZE, component::ReflectiveShadowData::SHADOW_SIZE);
				data.fluxDepth->setName("uFluxDepthSampler");

				MaterialProperties properties;
				properties.albedoColor       = glm::vec4(1.f, 1.f, 1.f, 1.f);
				properties.roughnessColor    = glm::vec4(0);
				properties.metallicColor     = glm::vec4(0);
				properties.usingAlbedoMap    = 0.0f;
				properties.usingRoughnessMap = 0.0f;
				properties.usingNormalMap    = 0.0f;
				properties.usingMetallicMap  = 0.0f;
				data.defaultMaterial          = std::make_shared<Material>(data.shader, properties);
				data.defaultMaterial->createDescriptorSet();
			});
		}

		auto registerShadowMap(ExecuteQueue &begin, ExecuteQueue &renderer, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerWithinQueue<reflective_shadow_map_pass::begin_scene::system>(begin);
			executePoint->registerWithinQueue<reflective_shadow_map_pass::onRender>(renderer);
		}
	};        // namespace reflective_shadow_map
};            // namespace maple
