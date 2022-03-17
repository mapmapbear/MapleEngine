//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "PropertiesWindow.h"
#include "Editor.h"
#include "Engine/GBuffer.h"
#include "Scene/Component/Atmosphere.h"
#include "Scene/Component/CameraControllerComponent.h"
#include "Scene/Component/Component.h"
#include "Scene/Component/Light.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Component/Sprite.h"
#include "Scene/Component/Transform.h"
#include "Scene/Component/VolumetricCloud.h"
#include "Scene/Component/LightProbe.h"

#include "Scene/Entity/Entity.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"

#include "Scripts/Mono/MonoComponent.h"
#include "Scripts/Mono/MonoScript.h"
#include "Scripts/Mono/MonoSystem.h"

#include "Engine/Renderer/PostProcessRenderer.h"
#include "Engine/Camera.h"
#include "Engine/Material.h"
#include "Engine/Mesh.h"
#include "ImGui/ImGuiHelpers.h"
#include "Others/Serialization.h"
#include "Others/StringUtils.h"

#include "ImGui/ImNotification.h"

#include "Inspector.inl"

#include <glm/gtc/type_ptr.hpp>

namespace MM
{
	namespace
	{
		std::string lightTypeToString(maple::component::LightType type)
		{
			switch (type)
			{
				case maple::component::LightType::DirectionalLight:
					return "Directional Light";
				case maple::component::LightType::SpotLight:
					return "Spot Light";
				case maple::component::LightType::PointLight:
					return "Point Light";
				default:
					return "ERROR";
			}
		}

		int32_t stringToLightType(const std::string &type)
		{
			if (type == "Directional")
				return int32_t(maple::component::LightType::DirectionalLight);

			if (type == "Point")
				return int32_t(maple::component::LightType::PointLight);

			if (type == "Spot")
				return int32_t(maple::component::LightType::SpotLight);
			return 0;
		}
	}        // namespace

	using namespace maple;

	template <>
	inline auto ComponentEditorWidget<component::SSAOData>( entt::registry& reg, entt::registry::entity_type e ) -> void
	{
		auto& ssao = reg.get<component::SSAOData>( e );
		ImGui::Columns( 2 );
		ImGui::Separator();
		ImGuiHelper::property( "SSAO Enable", ssao.enable );
		ImGuiHelper::property( "SSAO Radius", ssao.ssaoRadius,0.f,100.f);
		ImGui::Columns( 1 );
	}


	template <>
	inline auto ComponentEditorWidget<component::LPVGrid>(entt::registry& reg, entt::registry::entity_type e) -> void
	{
		auto& lpv = reg.get<component::LPVGrid>(e);
		ImGui::Columns(2);
		ImGui::Separator();
		ImGuiHelper::property("Indirect Light Attenuation", lpv.indirectLightAttenuation, 0.f, 1.f,maple::ImGuiHelper::PropertyFlag::InputFloat);
		ImGui::Columns(1);
	}
	
	template <>
	auto ComponentEditorWidget<component::Transform>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &transform = reg.get<component::Transform>(e);

		auto rotation = glm::degrees(transform.getLocalOrientation());
		auto position = transform.getLocalPosition();
		auto scale    = transform.getLocalScale();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
		ImGui::Columns(2);
		ImGui::Separator();

		ImGui::TextUnformatted("Position");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		if (ImGui::DragFloat3("##Position", glm::value_ptr(position), 0.05))
		{
			transform.setLocalPosition(position);
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::TextUnformatted("Rotation");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		if (ImGui::DragFloat3("##Rotation", glm::value_ptr(rotation), 0.1))
		{
			transform.setLocalOrientation(glm::radians(rotation));
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::TextUnformatted("Scale");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if (ImGui::DragFloat3("##Scale", glm::value_ptr(scale), 0.05))
		{
			transform.setLocalScale(scale);
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::Columns(1);
		ImGui::Separator();
		ImGui::PopStyleVar();
	}

	auto textureWidget(const char *label, Material *material, std::shared_ptr<Texture2D> tex, float &usingMapProperty, glm::vec4 &colorProperty, const std::function<void(const std::string &)> &callback, bool &update) -> void
	{
		if (ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
			ImGui::Columns(2);
			ImGui::Separator();

			ImGui::AlignTextToFramePadding();

			const ImGuiPayload *payload = ImGui::GetDragDropPayload();

			auto min = ImGui::GetCursorPos();
			auto max = min + ImVec2{64, 64} + ImGui::GetStyle().FramePadding;

			bool hoveringButton = ImGui::IsMouseHoveringRect(min, max, false);
			bool showTexture    = !(hoveringButton && (payload != NULL && payload->IsDataType("AssetFile")));
			if (tex && showTexture)
			{
				const bool flipImage = false;
				if (ImGui::ImageButton(tex->getHandle(), ImVec2{64, 64}, ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f)))
				{
				}

				if (ImGui::IsItemHovered() && tex)
				{
					ImGui::BeginTooltip();
					ImGui::Image(tex->getHandle(), ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
					ImGui::EndTooltip();
				}
			}
			else
			{
				if (ImGui::Button(tex ? "" : "Empty", ImVec2{64, 64}))
				{
				}
			}

			if (payload != NULL && payload->IsDataType("AssetFile"))
			{
				auto filePath = std::string(reinterpret_cast<const char *>(payload->Data));
				if (StringUtils::isTextureFile(filePath))
				{
					if (ImGui::BeginDragDropTarget())
					{
						// Drop directly on to node and append to the end of it's children list.
						if (ImGui::AcceptDragDropPayload("AssetFile"))
						{
							callback(filePath);
							update = true;
							ImGui::EndDragDropTarget();

							ImGui::Columns(1);
							ImGui::Separator();
							ImGui::PopStyleVar();

							ImGui::TreePop();
							return;
						}

						ImGui::EndDragDropTarget();
					}
				}
			}

			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::TextUnformatted(tex ? tex->getFilePath().c_str() : "No Texture");
			if (tex)
			{
				ImGuiHelper::tooltip(tex->getFilePath().c_str());
				ImGui::Text("%u x %u", tex->getWidth(), tex->getHeight());
				//ImGui::Text("Mip Levels : %u", tex->getMipmapLevel());
			}
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			if (ImGuiHelper::property("Use Map", usingMapProperty, 0.0f, 1.0f))
			{
				update = true;
			}
			if (ImGuiHelper::property("Color", colorProperty, 0.0f, 1.0f, false, ImGuiHelper::PropertyFlag::ColorProperty))
			{
				update = true;
			}

			ImGui::Columns(1);
			ImGui::PopStyleVar();

			ImGui::TreePop();
		}
	}

	template <>
	inline auto ComponentEditorWidget<component::Sprite>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &sprite = reg.get<component::Sprite>(e);

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
		ImGui::Columns(2);
		ImGui::Separator();

		/*auto pos = sprite.getQuad().getPosition();
		if (ImGuiHelper::property("Position", pos))
			sprite.getQuad().setPosition(pos);

		
	
		auto scale = sprite.getQuad().getScale();
		if (ImGuiHelper::property("Scale", scale))
			sprite.getQuad().setScale(scale);

	
		auto color = sprite.getQuad().getColor();
		if (ImGuiHelper::property("Colour", color,-1,1,false, ImGuiHelper::PropertyFlag::ColorProperty))
			sprite.getQuad().setColor(color);*/

		ImGui::Columns(1);

		if (ImGui::TreeNode("Texture"))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
			ImGui::Columns(2);
			ImGui::Separator();

			ImGui::AlignTextToFramePadding();
			auto tex = sprite.getQuad().getTexture();

			ImVec2 imageButtonSize(64, 64);

			auto                callback = std::bind(&component::Sprite::loadQuad, &sprite, std::placeholders::_1);
			const ImGuiPayload *payload  = ImGui::GetDragDropPayload();
			auto                min      = ImGui::GetCursorPos();
			auto                max      = min + imageButtonSize + ImGui::GetStyle().FramePadding;

			bool hoveringButton = ImGui::IsMouseHoveringRect(min, max, false);
			bool showTexture    = !(hoveringButton && (payload != NULL && payload->IsDataType("AssetFile")));
			bool flipImage      = false;
			if (tex && showTexture)
			{
				if (ImGui::ImageButton(tex->getHandle(), imageButtonSize, ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f)))
				{
				}

				if (ImGui::IsItemHovered() && tex)
				{
					ImGui::BeginTooltip();
					ImGui::Image(tex->getHandle(), ImVec2(256, 256), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
					ImGui::EndTooltip();
				}
			}
			else
			{
				if (ImGui::Button(tex ? "" : "Empty", imageButtonSize))
				{
				}
			}

			if (payload != NULL && payload->IsDataType("AssetFile"))
			{
				auto filePath = std::string(reinterpret_cast<const char *>(payload->Data));
				if (StringUtils::isTextureFile(filePath))
				{
					if (ImGui::BeginDragDropTarget())
					{
						// Drop directly on to node and append to the end of it's children list.
						if (ImGui::AcceptDragDropPayload("AssetFile"))
						{
							callback(filePath);
							ImGui::EndDragDropTarget();

							ImGui::Columns(1);
							ImGui::Separator();
							ImGui::PopStyleVar(2);

							ImGui::TreePop();
							return;
						}

						ImGui::EndDragDropTarget();
					}
				}
			}

			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			ImGui::TextUnformatted(tex ? tex->getFilePath().c_str() : "No Texture");
			if (tex)
			{
				ImGuiHelper::tooltip(tex->getFilePath().c_str());
				ImGui::Text("%u x %u", tex->getWidth(), tex->getHeight());
				//	ImGui::Text("Mip Levels : %u", tex->getMipmapLevel());
			}

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::Columns(1);
			ImGui::Separator();
			ImGui::PopStyleVar();
			ImGui::TreePop();
		}

		ImGui::Columns(1);
		ImGui::Separator();
		ImGui::PopStyleVar();
	}

	template <>
	inline auto ComponentEditorWidget<component::AnimatedSprite>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &sprite = reg.get<component::AnimatedSprite>(e);

		ImGui::Columns(2);

		auto frame = sprite.getCurrentId();
		if (ImGuiHelper::property("Frame", frame, 0, sprite.getFrames() - 1))
		{
			sprite.setCurrentFrame(frame);
		}
		bool loop = sprite.isLoop();
		if (ImGuiHelper::property("Loop", loop))
		{
			sprite.setLoop(loop);
		}

		auto delay = sprite.getDelay();
		if (ImGuiHelper::inputFloat("Delay", delay))
		{}

		auto timer = sprite.getTimer();
		if (ImGuiHelper::inputFloat("Timer", timer))
		{}

		ImGui::Columns(1);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
		ImGui::Separator();
		ImGui::AlignTextToFramePadding();
		auto tex    = sprite.getQuad().getTexture();
		auto coords = sprite.getQuad().getTexCoords();

		auto &color = sprite.getQuad().getColor();

		if (tex)
		{
			auto w = (float) sprite.getQuad().getWidth();
			auto h = (float) sprite.getQuad().getHeight();

			if (w > 256)
			{
				h = 256.f / w * h;
				w = 256;
			}

			ImGui::Image(tex->getHandle(), {w, h},
			             ImVec2(coords[3].x, coords[3].y),
			             ImVec2(coords[1].x, coords[1].y),
			             {color.r, color.g, color.b, color.a});
		}

		ImGui::PopStyleVar();
	}

	template <>
	inline auto ComponentEditorWidget<component::MeshRenderer>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &mesh = reg.get<component::MeshRenderer>(e);

		auto & materials = mesh.getMesh()->getMaterial();

		const std::string matName = "Material";
		if (materials.empty())
		{
			ImGui::TextUnformatted("Empty Material");
			if (ImGui::Button("Add Material", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
				mesh.getMesh()->setMaterial(std::make_shared<Material>());
		}
		else 
		{
			for (auto i = 0;i< materials.size();i++)
			{
				auto& material = materials[i];
				std::string name = matName + "_" + std::to_string(i);
				if (ImGui::TreeNodeEx(name.c_str(), 0))
				{
					auto& prop = material->getProperties();
					auto  color = glm::vec4(0.f);
					auto  textures = material->getTextures();

					bool update = false;

					MM::textureWidget("Albedo", material.get(), textures.albedo, prop.usingAlbedoMap, prop.albedoColor, std::bind(&Material::setAlbedoTexture, material, std::placeholders::_1), update);
					ImGui::Separator();

					MM::textureWidget("Normal", material.get(), textures.normal, prop.usingNormalMap, color, std::bind(&Material::setNormalTexture, material, std::placeholders::_1), update);
					ImGui::Separator();

					MM::textureWidget("Metallic", material.get(), textures.metallic, prop.usingMetallicMap, prop.metallicColor, std::bind(&Material::setMetallicTexture, material, std::placeholders::_1), update);
					ImGui::Separator();

					MM::textureWidget("Roughness", material.get(), textures.roughness, prop.usingRoughnessMap, prop.roughnessColor, std::bind(&Material::setRoughnessTexture, material, std::placeholders::_1), update);
					ImGui::Separator();

					MM::textureWidget("AO", material.get(), textures.ao, prop.usingAOMap, color, std::bind(&Material::setAOTexture, material, std::placeholders::_1), update);
					ImGui::Separator();

					MM::textureWidget("Emissive", material.get(), textures.emissive, prop.usingEmissiveMap, prop.emissiveColor, std::bind(&Material::setEmissiveTexture, material, std::placeholders::_1), update);

					if (update)
						material->setMaterialProperites(prop);

					ImGui::TreePop();
				}
			}
		}

		ImGui::Columns(2);
		ImGui::Separator();

		ImGuiHelper::property("Cast Shadow", mesh.castShadow);

		ImGui::Columns(1);

	}

	template <>
	inline auto ComponentEditorWidget<component::Light>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &light = reg.get<component::Light>(e);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
		ImGui::Columns(2);
		ImGui::Separator();

		if (static_cast<component::LightType>(light.lightData.type) != component::LightType::DirectionalLight)
			ImGuiHelper::property("Radius", light.lightData.radius, 1.0f, 100.0f);

		ImGuiHelper::property("Color", light.lightData.color, true, ImGuiHelper::PropertyFlag::ColorProperty);
		ImGuiHelper::property("Intensity", light.lightData.intensity, 0.0f, 100.0f);

		if (static_cast<component::LightType>(light.lightData.type) == component::LightType::SpotLight)
			ImGuiHelper::property("Angle", light.lightData.angle, -1.0f, 1.0f);

		ImGuiHelper::property("Show Frustum", light.showFrustum);
		ImGuiHelper::property("Reflective Shadow Map", light.reflectiveShadowMap);
		ImGuiHelper::property("Light Propagation Volume", light.enableLPV);
		ImGuiHelper::property("Cast Shadow", light.castShadow);


		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Light Type");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		const char *lights[]  = {"Directional Light", "Spot Light", "Point Light"};
		auto        currLight = lightTypeToString(component::LightType(int32_t(light.lightData.type)));

		if (ImGui::BeginCombo("LightType", currLight.c_str(), 0))
		{
			for (auto n = 0; n < 3; n++)
			{
				if (ImGui::Selectable(lights[n]))
				{
					light.lightData.type = n;
				}
			}
			ImGui::EndCombo();
		}

		ImGui::Columns(1);
		ImGui::Separator();
		ImGui::PopStyleVar();
	}

	template <>
	inline auto ComponentEditorWidget<component::MonoComponent>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &mono = reg.get<component::MonoComponent>(e);
		ImGui::PushID("add c# script");
		static std::string path = "scripts/";
		ImGui::PushItemWidth(-1);
		if (ImGui::BeginCombo("", "", 0))
		{
			for (const auto &entry : std::filesystem::directory_iterator(path))
			{
				bool isDir = std::filesystem::is_directory(entry);
				if (StringUtils::isCSharpFile(entry.path().string()))
				{
					if (ImGui::Selectable(entry.path().string().c_str()))
					{
						mono.addScript(entry.path().string(), Application::get()->getSystemManager()->getSystem<MonoSystem>());
					}
				}
			}
			ImGui::EndCombo();
		}
		ImGui::PopID();
	}

	template <>
	inline auto ComponentEditorWidget<component::Environment>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &env = reg.get<component::Environment>(e);

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
		ImGui::Columns(2);
		ImGui::Separator();

		ImGui::TextUnformatted("Skybox type");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		static char *boxItems[2] = {"Environment", "PseudoSky"};
		if (ImGui::BeginCombo("", !env.isPseudoSky() ? "Environment" : "PseudoSky", 0))
		{
			for (auto n = 0; n < 2; n++)
			{
				if (ImGui::Selectable(boxItems[n], true))
				{
					env.setPseudoSky(n == 1);
				}
			}
			ImGui::EndCombo();
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		if (!env.isPseudoSky())
		{
			auto label = env.getFilePath();

			auto updated = ImGuiHelper::property("File", label, true);
			if (ImGui::BeginDragDropTarget())
			{
				auto data = ImGui::AcceptDragDropPayload("AssetFile", ImGuiDragDropFlags_None);
				if (data)
				{
					std::string file = (char *) data->Data;
					if (StringUtils::isTextureFile(file))
					{
						env.init(file);
					}
				}
				ImGui::EndDragDropTarget();
			}
			if (updated)
			{
				ImGui::Columns(1);
				ImGui::Separator();

				if (env.getEquirectangularMap())
				{
					ImGuiHelper::image(env.getEquirectangularMap().get(), {64, 64});
				}
			}
		}
		else
		{
			ImGuiHelper::property("SkyColor Top", env.getSkyColorTop(), ImGuiHelper::PropertyFlag::ColorProperty);
			ImGuiHelper::property("SkyColor Bottom", env.getSkyColorBottom(), ImGuiHelper::PropertyFlag::ColorProperty);
		}

		ImGui::Columns(1);
		ImGui::Separator();
		ImGui::PopStyleVar();
	}

	template <>
	inline auto ComponentEditorWidget<Camera>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &camera = reg.get<Camera>(e);

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
		ImGui::Columns(2);
		ImGui::Separator();

		float aspect = camera.getAspectRatio();

		if (ImGuiHelper::property("Aspect", aspect, 0.0f, 10.0f))
			camera.setAspectRatio(aspect);

		float fov = camera.getFov();
		if (ImGuiHelper::property("Fov", fov, 1.0f, 120.0f))
			camera.setFov(fov);

		float near_ = camera.getNear();
		if (ImGuiHelper::inputFloat("Near", near_, 0.01, 10.f))
			camera.setNear(near_);

		float far_ = camera.getFar();
		if (ImGuiHelper::inputFloat("Far", far_, 10.0f, 1000.0f))
			camera.setFar(far_);

		float scale = camera.getScale();
		if (ImGuiHelper::inputFloat("Scale", scale, 0.0f, 10000.f))
			camera.setScale(scale);

		bool ortho = camera.isOrthographic();
		if (ImGuiHelper::property("Orthographic", ortho))
			camera.setOrthographic(ortho);

		ImGui::Columns(1);
		ImGui::Separator();
		ImGui::PopStyleVar();
	}

	template <>
	inline auto ComponentEditorWidget<component::CameraControllerComponent>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &controllerComp = reg.get<component::CameraControllerComponent>(e);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
		ImGui::Columns(2);
		ImGui::Separator();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Controller Type");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		const std::array<std::string, 2> controllerTypes   = {"FPS", "Editor"};
		std::string                      currentController = component::CameraControllerComponent::typeToString(controllerComp.getType());
		if (ImGui::BeginCombo("", currentController.c_str(), 0))        // The second parameter is the label previewed before opening the combo.
		{
			for (auto n = 0; n < controllerTypes.size(); n++)
			{
				bool isSelected = currentController == controllerTypes[n];
				if (ImGui::Selectable(controllerTypes[n].c_str(), currentController.c_str()))
				{
					controllerComp.setControllerType(component::CameraControllerComponent::stringToType(controllerTypes[n]));
				}
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		if (controllerComp.getController())
			controllerComp.getController()->onImGui();

		ImGui::Columns(1);
		ImGui::Separator();
		ImGui::PopStyleVar();
	}

	template <>
	inline auto ComponentEditorWidget<component::Atmosphere>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &atmosphere = reg.get<component::Atmosphere>(e);
		auto &data       = atmosphere.getData();

		ImGui::Columns(3);
		ImGui::Separator();

		ImGuiHelper::propertyWithDefault("Surface Radius", atmosphere.getData().surfaceRadius, 1.f, 1.f, component::Atmosphere::SURFACE_RADIUS, ImGuiHelper::PropertyFlag::InputFloat);
		ImGuiHelper::propertyWithDefault("Atmosphere Radius", atmosphere.getData().atmosphereRadius, 1.f, 1.f, component::Atmosphere::ATMOSPHE_RERADIUS, ImGuiHelper::PropertyFlag::InputFloat);

		ImGuiHelper::propertyWithDefault("Center Point", atmosphere.getData().centerPoint, -10000.f, 0.f, {0.f, -component::Atmosphere::SURFACE_RADIUS, 0.f}, ImGuiHelper::PropertyFlag::InputFloat);
		auto mie = 10000.f * atmosphere.getData().mieScattering;

		if (ImGuiHelper::propertyWithDefault("Mie Scattering", mie, 0.f, 10.f, component::Atmosphere::MIE_SCATTERING * 10000.f, ImGuiHelper::PropertyFlag::InputFloat, 0.01))
		{
			atmosphere.getData().mieScattering = mie / 10000.f;
		}
		auto ray = 10000.f * atmosphere.getData().rayleighScattering;

		if (ImGuiHelper::propertyWithDefault("Ray Scattering", ray, 0.f, 10.f, component::Atmosphere::RAYLEIGH_SCATTERING * 10000.f, ImGuiHelper::PropertyFlag::InputFloat, 0.01))
		{
			atmosphere.getData().rayleighScattering = ray / 10000.f;
		}
		ImGuiHelper::propertyWithDefault("Anisotropy(g)", atmosphere.getData().g, 0.f, 1.f, component::Atmosphere::G, ImGuiHelper::PropertyFlag::InputFloat, 0.01);

		ImGui::Columns(1);
		ImGui::Separator();
	}

	template <>
	inline auto ComponentEditorWidget<component::VolumetricCloud>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &cloud = reg.get<component::VolumetricCloud>(e);

		ImGui::Columns(2);
		ImGui::Separator();

		ImGuiHelper::property("Post Processing (Gaussian Blur)", cloud.postProcess);
		ImGuiHelper::property("Light Scatter", cloud.enableGodRays);
		ImGuiHelper::property("Enable sugar powder effect", cloud.enablePowder);

		ImGuiHelper::property("Coverage", cloud.coverage, 0.0f, 1.0f);
		if (ImGuiHelper::property("Speed", cloud.cloudSpeed, 0.0f, 5.0E3))
		{
			cloud.weathDirty = true;
		}
		ImGuiHelper::property("Crispiness", cloud.crispiness, 0.0f, 120.0f);
		ImGuiHelper::property("Curliness", cloud.curliness, 0.0f, 3.0f);
		ImGuiHelper::property("Density", cloud.density, 0.0f, 0.1f);
		ImGuiHelper::property("Light Absorption", cloud.absorption, 0.0f, 1.5f);

		if (ImGuiHelper::property("Clouds frequency", cloud.perlinFrequency, 0.0f, 4.0f))
		{
			// -> noise map
			cloud.weathDirty = true;
		}

		ImGuiHelper::property("Sky dome radius", cloud.earthRadius, 10000.0f, 5000000.0f);
		ImGuiHelper::property("Clouds bottom height", cloud.sphereInnerRadius, 1000.0f, 15000.0f);
		ImGuiHelper::property("Clouds top height", cloud.sphereOuterRadius, 1000.0f, 40000.0f);

		ImGui::Columns(1);
		ImGui::Separator();
	}

};        // namespace MM

namespace maple
{
	constexpr size_t INPUT_BUFFER = 256;
	PropertiesWindow::PropertiesWindow()
	{
		
	}

	auto PropertiesWindow::onImGui() -> void
	{
		auto &editor           = static_cast<Editor &>(*Application::get());
		auto &registry         = editor.getSceneManager()->getCurrentScene()->getRegistry();
		auto  selected         = editor.getSelected();
		auto  selectedResource = editor.getEditingResource();
		if (selectedResource == "")
		{
			selectedResource = editor.getSelectResource();
		}

		if (ImGui::Begin(STATIC_NAME, &active))
		{
			if (selected != entt::null)
			{
				auto activeComponent = registry.try_get<component::ActiveComponent>(selected);
				bool active          = activeComponent ? activeComponent->active : true;
				if (ImGui::Checkbox("##ActiveCheckbox", &active))
				{
					if (!activeComponent)
						registry.emplace<component::ActiveComponent>(selected, active);
					else
						activeComponent->active = active;
				}
				ImGui::SameLine();
				ImGui::TextUnformatted(ICON_MDI_CUBE);
				ImGui::SameLine();

				bool        hasName = registry.has<component::NameComponent>(selected);
				std::string name;
				if (hasName)
					name = registry.get<component::NameComponent>(selected).name;
				else
					name = std::to_string(entt::to_integral(selected));

				static char objName[INPUT_BUFFER];
				strcpy(objName, name.c_str());

				if (ImGui::InputText("##Name", objName, IM_ARRAYSIZE(objName)))
					registry.get_or_emplace<component::NameComponent>(selected).name = objName;

				ImGui::Separator();

				enttEditor.renderEditor(registry, selected);

				if (ImGui::BeginDragDropTarget())
				{
					auto data = ImGui::AcceptDragDropPayload("AssetFile", ImGuiDragDropFlags_None);
					if (data)
					{
					}
					ImGui::EndDragDropTarget();
				}
			}
			else if (selectedResource != "")
			{
				drawResource(selectedResource);
			}
		}
		ImGui::End();
	}

	auto PropertiesWindow::onSceneCreated(Scene *scene) -> void
	{
		enttEditor.clear();
		auto &editor  = static_cast<Editor &>(*Application::get());
		registerInspector(enttEditor);
		MM::EntityEditor<entt::entity>::ComponentInfo info;
		info.hasChildren = true;
		info.childrenDraw = [](entt::registry &registry, entt::entity ent) {
			auto mono = registry.try_get<component::MonoComponent>(ent);

			if (mono)
			{
				for (auto &script : mono->getScripts())
				{
					ImGui::PushID(script.first.c_str());
					if (ImGui::Button("-"))
					{
						mono->remove(script.first);
						ImGui::PopID();
						continue;
					}
					else
					{
						ImGui::SameLine();
					}

					if (ImGui::CollapsingHeader(script.second->getClassNameInEditor().c_str(), ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::Indent(30.f);
						ImGui::PushID("Widget");
						//	ci.widget(registry, e);

						ImGui::PopID();
						ImGui::Unindent(30.f);
					}
					ImGui::PopID();
				}
			}
		};
		enttEditor.registerComponent<component::MonoComponent>(info);
		enttEditor.acceptFile = [&](const std::string &fileName, entt::registry &registry, entt::entity &ent) {
			if (StringUtils::isCSharpFile(fileName))
			{
				auto &mono = registry.get_or_emplace<component::MonoComponent>(ent);
				mono.setEntity(ent);
				mono.addScript(fileName, Application::get()->getSystemManager()->getSystem<MonoSystem>());
			}
		};
	}

	auto PropertiesWindow::drawResource(const std::string &path) -> void
	{
		ImGui::TextUnformatted(StringUtils::getFileName(path).c_str());

		ImGui::Separator();

		if (File::isKindOf(path, FileType::Material))
		{
			auto material = Application::getCache()->emplace<Material>(path);

			std::string matName = "Material";

			auto &prop     = material->getProperties();
			auto  color    = glm::vec4(0.f);
			auto  textures = material->getTextures();

			bool update = false;

			MM::textureWidget("Albedo", material.get(), textures.albedo, prop.usingAlbedoMap, prop.albedoColor, std::bind(&Material::setAlbedoTexture, material, std::placeholders::_1), update);
			ImGui::Separator();

			MM::textureWidget("Normal", material.get(), textures.normal, prop.usingNormalMap, color, std::bind(&Material::setNormalTexture, material, std::placeholders::_1), update);
			ImGui::Separator();

			MM::textureWidget("Metallic", material.get(), textures.metallic, prop.usingMetallicMap, prop.metallicColor, std::bind(&Material::setMetallicTexture, material, std::placeholders::_1), update);
			ImGui::Separator();

			MM::textureWidget("Roughness", material.get(), textures.roughness, prop.usingRoughnessMap, prop.roughnessColor, std::bind(&Material::setRoughnessTexture, material, std::placeholders::_1), update);
			ImGui::Separator();

			MM::textureWidget("AO", material.get(), textures.ao, prop.usingAOMap, color, std::bind(&Material::setAOTexture, material, std::placeholders::_1), update);
			ImGui::Separator();

			MM::textureWidget("Emissive", material.get(), textures.emissive, prop.usingEmissiveMap, prop.emissiveColor, std::bind(&Material::setEmissiveTexture, material, std::placeholders::_1), update);

			if (update)
				material->setMaterialProperites(prop);
			ImGui::Separator();

			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.f);
			if (ImGui::Button("Save Material"))
			{
				Serialization::serialize(material.get());
				ImNotification::makeNotification("Tips", "save success", ImNotification::Type::Success);
			}
			ImGui::PopStyleVar();
		}
	}

};        // namespace maple
