///////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "DeferredOffScreenRenderer.h"

#include "RHI/GPUProfile.h"
#include "RHI/Pipeline.h"
#include "RHI/Shader.h"
#include "RHI/Texture.h"

#include "Engine/Camera.h"
#include "Engine/GBuffer.h"
#include "Engine/Mesh.h"
#include "Engine/Profiler.h"

#include "Scene/Component/Light.h"
#include "Scene/Component/Transform.h"
#include "Scene/Scene.h"

#include "Others/Randomizer.h"

#include "ImGui/ImGuiHelpers.h"

#include "Application.h"

#include "RendererData.h"

#include <glm/gtc/type_ptr.hpp>

#include <ecs/ecs.h>

namespace maple
{
	namespace component
	{
		DeferredData::DeferredData()
		{
			deferredColorShader = Shader::create("shaders/DeferredColor.shader");
			deferredLightShader = Shader::create("shaders/DeferredLight.shader");

			commandQueue.reserve(1000);

			MaterialProperties properties;
			properties.albedoColor       = glm::vec4(1.f, 1.f, 1.f, 1.f);
			properties.roughnessColor    = glm::vec4(0);
			properties.metallicColor     = glm::vec4(0);
			properties.usingAlbedoMap    = 0.0f;
			properties.usingRoughnessMap = 0.0f;
			properties.usingNormalMap    = 0.0f;
			properties.usingMetallicMap  = 0.0f;

			defaultMaterial = std::make_shared<Material>(deferredColorShader, properties);
			defaultMaterial->createDescriptorSet();

			DescriptorInfo info{};
			descriptorColorSet.resize(3);
			descriptorLightSet.resize(1);

			info.shader           = deferredColorShader.get();
			info.layoutIndex      = 0;
			descriptorColorSet[0] = DescriptorSet::create(info);

			info.layoutIndex      = 2;
			descriptorColorSet[2] = DescriptorSet::create(info);

			info.shader           = deferredLightShader.get();
			info.layoutIndex      = 0;
			descriptorLightSet[0] = DescriptorSet::create(info);
			screenQuad            = Mesh::createQuad(true);
		}
	}        // namespace component

	namespace deferred_offscreen
	{
		using Entity = ecs::Chain
			::Write<component::DeferredData>
			::Read<component::CameraView>
			::To<ecs::Entity>;

		using LightDefine = ecs::Chain
			::Write<Light>
			::Read<Transform>;

		using Query = LightDefine
			::To<ecs::Query>;

		using LightEntity = LightDefine
			::To<ecs::Entity>;

		inline auto beginScene(Entity entity, Query lightQuery, ecs::World world)
		{
			auto [data, cameraView] = entity;
			data.commandQueue.clear();
			auto descriptorSet = data.descriptorColorSet[0];

			data.descriptorColorSet[0]->setUniform("UniformBufferObject", "projView", &cameraView.projView);
			data.descriptorColorSet[0]->setUniform("UniformBufferObject", "view", &cameraView.view);
			data.descriptorColorSet[0]->setUniform("UniformBufferObject", "projViewOld", &cameraView.projViewOld);

			data.descriptorColorSet[2]->setUniform("UBO", "view", &cameraView.view);
			data.descriptorColorSet[2]->setUniform("UBO", "nearPlane", &cameraView.nearPlane);
			data.descriptorColorSet[2]->setUniform("UBO", "farPlane", &cameraView.farPlane);

			Light *directionaLight = nullptr;

			LightData lights[32] = {};
			uint32_t  numLights  = 0;

			Light *    rsmLighting  = nullptr;
			Transform *rsmTransform = nullptr;

			lightQuery.forEach([](LightEntity entity) {

			});

			/*{
				PROFILE_SCOPE("Get Light");
				auto group = registry.group<Light>(entt::get<Transform>);

				for (auto &lightEntity : group)
				{
					const auto &[light, trans] = group.get<Light, Transform>(lightEntity);
					light.lightData.position   = {trans.getWorldPosition(), 1.f};
					light.lightData.direction  = {glm::normalize(trans.getWorldOrientation() * maple::FORWARD), 1.f};

					if (static_cast<LightType>(light.lightData.type) == LightType::DirectionalLight)
						directionaLight = &light;

					if (static_cast<LightType>(light.lightData.type) != LightType::DirectionalLight)
					{
						/ *auto inside = forwardData->frustum.isInsideFast(Sphere(light.lightData.position, light.lightData.radius));

					if (inside == Intersection::Outside)
						continue;* /
					}

					if (light.reflectiveShadowMap)
					{
						rsmLighting  = &light;
						rsmTransform = &trans;
					}

					lights[numLights] = light.lightData;
					numLights++;
				}
			}

			auto descriptorSet = settings.deferredRender ? deferredData->descriptorLightSet[0] : forwardData->descriptorSet[2];
			descriptorSet->setUniform("UniformBufferLight", "lights", lights, sizeof(LightData) * numLights, false);
			auto cameraPos = glm::vec4{camera.second->getWorldPosition(), 1.f};
			descriptorSet->setUniform("UniformBufferLight", "cameraPosition", &cameraPos);*/
		}

		inline auto onRender(Entity entity, ecs::World world)
		{
			//auto [data, cameraView] = entity;
		}

		auto registerDeferredOffScreenRenderer(ExecuteQueue &begin, ExecuteQueue &renderer, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerGlobalComponent<component::DeferredData>();
			executePoint->registerWithinQueue<deferred_offscreen::beginScene>(begin);
			executePoint->registerWithinQueue<deferred_offscreen::onRender>(renderer);
		}
	}        // namespace deferred_offscreen
};           // namespace maple
