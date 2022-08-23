//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "PropertiesWindow.h"
#include "Editor.h"
#include "Engine/AmbientOcclusion/SSAORenderer.h"
#include "Engine/Camera.h"
#include "Engine/DDGI/DDGIRenderer.h"
#include "Engine/GBuffer.h"
#include "Engine/Material.h"
#include "Engine/Mesh.h"
#include "Engine/PathTracer/PathIntegrator.h"
#include "Engine/Raytrace/RaytracedShadow.h"
#include "Engine/Renderer/GridRenderer.h"
#include "Engine/Renderer/PostProcessRenderer.h"
#include "Engine/Renderer/ShadowRenderer.h"
#include "Engine/Renderer/SkyboxRenderer.h"

#include "Scene/Component/Atmosphere.h"
#include "Scene/Component/Bindless.h"
#include "Scene/Component/BoundingBox.h"
#include "Scene/Component/CameraControllerComponent.h"
#include "Scene/Component/Component.h"
#include "Scene/Component/Environment.h"
#include "Scene/Component/Light.h"
#include "Scene/Component/LightProbe.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Component/Transform.h"
#include "Scene/Component/VolumetricCloud.h"

#include "Scene/System/EnvironmentModule.h"

#include "Scene/Entity/Entity.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"

#include "2d/Sprite.h"
#include "Animation/Animation.h"
#include "Animation/AnimationSystem.h"

#include "Scripts/Mono/MonoComponent.h"
#include "Scripts/Mono/MonoModule.h"
#include "Scripts/Mono/MonoScript.h"
#include "Scripts/Mono/MonoSystem.h"

#include "Physics/Collider.h"
#include "Physics/PhysicsSystem.h"
#include "Physics/PhysicsWorld.h"
#include "Physics/RigidBody.h"

#include "FileSystem/Skeleton.h"
#include "Loaders/Loader.h"

#include "ImGui/ImGuiHelpers.h"
#include "ImGui/ImNotification.h"

#include "Others/Serialization.h"
#include "Others/StringUtils.h"

#include "Inspector.inl"

#include "CurveWindow.h"

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
	inline auto ComponentEditorWidget<ddgi::component::DDGIPipeline>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &comp = reg.get<ddgi::component::DDGIPipeline>(e);

		ImGui::Columns(2);
		ImGui::Separator();

		bool updated = false;

		updated = updated || ImGuiHelper::property("Probe Distance", comp.probeDistance, 1, 100);
		updated = updated || ImGuiHelper::property("Infinite Bounce", comp.infiniteBounce);
		updated = updated || ImGuiHelper::property("Rays Per Probe", comp.raysPerProbe, 64, 512);
		updated = updated || ImGuiHelper::property("Hysteresis", comp.hysteresis, 0.0f, 10.f);
		updated = updated || ImGuiHelper::property("GI Intensity", comp.intensity, 1.f, 10.f);
		updated = updated || ImGuiHelper::property("Infinite Bounce Intensity", comp.infiniteBounceIntensity, 1.f, 10.f);
		updated = updated || ImGuiHelper::property("Normal Bias", comp.normalBias, 0.f, 5.f);
		updated = updated || ImGuiHelper::property("Depth Sharpness", comp.depthSharpness, 50.f, 100.f);
		updated = updated || ImGuiHelper::property("Energy Preservation", comp.energyPreservation, 0.f, 2.f);

		if (auto id = ImGuiHelper::combox("Raytrace Scale", RaytraceScale::Names, RaytraceScale::Length, comp.scale); id != -1)
		{
			comp.scale = static_cast<RaytraceScale::Id>(id);
			updated    = true;
		}

		if (updated)
		{        //update uniform.....
			reg.patch<ddgi::component::DDGIPipeline>(e);
		}

		ImGui::Separator();
		ImGui::Columns(1);

		if (ImGui::Button("Apply GI"))
		{
			reg.emplace<ddgi::component::ApplyEvent>(e);
		}
		ImGui::Separator();
	}

	template <>
	inline auto ComponentEditorWidget<raytraced_shadow::component::RaytracedShadow>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &shadow = reg.get<raytraced_shadow::component::RaytracedShadow>(e);

		ImGui::Columns(2);
		ImGui::Separator();

		if (auto id = ImGuiHelper::combox("RaytraceScale", RaytraceScale::Names, RaytraceScale::Length, shadow.scale); id != -1)
		{
			shadow.scale = static_cast<RaytraceScale::Id>(id);
		}

		ImGui::Separator();
		ImGui::Columns(1);
	}

	template <>
	inline auto ComponentEditorWidget<vxgi::component::Voxelization>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &voxel = reg.get<vxgi::component::Voxelization>(e);

		ImGui::Columns(2);
		ImGui::Separator();

		ImGuiHelper::showProperty("Dirty", std::to_string(voxel.dirty));
		ImGuiHelper::showProperty("InjectFirstBounce", std::to_string(voxel.injectFirstBounce));
		ImGuiHelper::showProperty("VoxelSize", std::to_string(voxel.voxelSize));
		ImGuiHelper::showProperty("VolumeGridSize", std::to_string(voxel.volumeGridSize));
		ImGuiHelper::property("Max Tracing Distance", voxel.maxTracingDistance, 1, 10);
		ImGuiHelper::property("AO Falloff", voxel.aoFalloff);
		ImGuiHelper::property("Sampling Factor", voxel.samplingFactor);
		ImGuiHelper::property("Bounce Strength", voxel.bounceStrength, 0, 10);
		ImGuiHelper::property("AO Alpha", voxel.aoAlpha);
		ImGuiHelper::property("Trace Shadow Hit", voxel.traceShadowHit);
		ImGui::Separator();
		ImGui::Columns(1);
	}

	template <>
	inline auto ComponentEditorWidget<component::PathIntegrator>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &path = reg.get<component::PathIntegrator>(e);

		ImGui::Columns(2);
		ImGui::Separator();

		ImGuiHelper::showProperty("Accumulated Samples", std::to_string(path.accumulatedSamples));

		ImGuiHelper::property("Shadow Ray Bias", path.shadowRayBias);

		ImGuiHelper::image(path.images[0].get(), {80, 45});
		ImGuiHelper::image(path.images[1].get(), {80, 45});

		ImGui::Separator();
		ImGui::Columns(1);
	}

	template <>
	inline auto ComponentEditorWidget<vxgi_debug::global::component::DrawVoxelRender>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &voxel = reg.get<vxgi_debug::global::component::DrawVoxelRender>(e);

		ImGui::Columns(2);
		ImGui::Separator();

		ImGuiHelper::property("Enable", voxel.enable);

		ImGuiHelper::property("Visualize Mipmap", voxel.drawMipmap);

		if (auto id = ImGuiHelper::combox("Voxel BufferId", VoxelBufferId::Names, VoxelBufferId::Length, voxel.id); id != -1)
		{
			voxel.id = static_cast<VoxelBufferId::Id>(id);
		}

		if (voxel.drawMipmap)
		{
			voxel.id = VoxelBufferId::Id::Radiance;
			ImGuiHelper::property("Direction", voxel.direction, 0, 5);
			ImGuiHelper::property("Mipmap", voxel.mipLevel, 0, 7);
		}

		ImGuiHelper::property("Color Channels", voxel.colorChannels, 0.f, 1.f, true);

		ImGui::Separator();
		ImGui::Columns(1);
	}

	template <>
	inline auto ComponentEditorWidget<physics::component::Collider>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &collider  = reg.get<physics::component::Collider>(e);
		auto  rigidBody = reg.try_get<physics::component::RigidBody>(e);

		ImGui::Columns(1);
		ImGui::TextUnformatted(physics::getNameByType(collider.type));
		if (collider.type == physics::ColliderType::BoxCollider)
		{
			ImGui::Columns(2);
			ImGui::Separator();

			auto oldSize = collider.box.size();
			auto size    = oldSize;

			auto oldCenter = collider.box.center();
			auto center    = oldCenter;

			if (ImGuiHelper::property("Center", center, 0, 0, ImGuiHelper::PropertyFlag::DragFloat))
			{
				auto delta = center - oldCenter;
				collider.box.min += delta;
				collider.box.max += delta;
			}

			if (ImGuiHelper::property("Extend", size, 0, 0, ImGuiHelper::PropertyFlag::DragFloat))
			{
				auto deltaSize = (size - oldSize) / 2.f;
				collider.box.min -= deltaSize;
				collider.box.max += deltaSize;
			}

			ImGui::Separator();
		}
		else if (collider.type == physics::ColliderType::SphereCollider)
		{
			ImGui::Columns(2);
			ImGui::Separator();
			ImGuiHelper::property("Radius", collider.radius, 0, 0, ImGuiHelper::PropertyFlag::DragFloat);
			ImGui::Separator();
		}
		else if (collider.type == physics::ColliderType::CapsuleCollider)
		{
			ImGui::Columns(2);
			ImGui::Separator();
			ImGuiHelper::property("Radius", collider.radius, 0, 0, ImGuiHelper::PropertyFlag::DragFloat);
			ImGuiHelper::property("Height", collider.height, 0, 0, ImGuiHelper::PropertyFlag::DragFloat);

			auto oldCenter = collider.box.center();
			auto center    = oldCenter;
			if (ImGuiHelper::property("Center", center, 0, 0, ImGuiHelper::PropertyFlag::DragFloat))
			{
				auto delta = center - oldCenter;
				collider.box.min += delta;
				collider.box.max += delta;
			}

			ImGui::Separator();
		}
		ImGui::Columns(1);
		ImGui::Separator();
	}

	template <>
	inline auto ComponentEditorWidget<physics::component::RigidBody>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &transform = reg.get<component::Transform>(e);
		auto &rigidRody = reg.get<physics::component::RigidBody>(e);
		auto  collider  = reg.try_get<physics::component::Collider>(e);
		ImGui::Columns(2);
		ImGui::Separator();

		ImGuiHelper::property("Kinematic", rigidRody.kinematic);

		if (ImGuiHelper::property("Dynamic", rigidRody.dynamic))
		{
		}

		if (ImGuiHelper::property("Mass", rigidRody.mass, 0, 0, maple::ImGuiHelper::PropertyFlag::DragFloat))
		{
			if (rigidRody.mass == 0.0f)
			{
				rigidRody.dynamic = false;
			}
		}

		ImGui::Separator();
		ImGui::Columns(1);

		if (ImGui::TreeNode("RigidBody-Info"))
		{
			ImGui::Columns(2);
			ImGuiHelper::showProperty("Local Inertia", rigidRody.localInertia);
			ImGuiHelper::showProperty("World Center Mass", rigidRody.worldCenterPositionMass);
			ImGuiHelper::showProperty("Velocity", rigidRody.velocity);
			ImGuiHelper::showProperty("Angular Velocity", rigidRody.angularVelocity);
			ImGui::TreePop();
		}

		ImGui::Columns(1);
	}

	template <>
	inline auto ComponentEditorWidget<component::BoundingBoxComponent>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &bbbox = reg.get<component::BoundingBoxComponent>(e);
		auto transform = reg.try_get<component::Transform>(e);

		if (auto box = bbbox.box; box != nullptr && transform)
		{
			ImGui::Separator();
			ImGui::Columns(1);
			ImGui::TextUnformatted("Bounds");
			ImGui::Columns(2);

			auto newBB = box->transform(transform->getWorldMatrix());

			auto size   = newBB.size();
			auto center = newBB.center();

			//show size is ok.

			ImGuiHelper::property("Center", center, 0, 0, ImGuiHelper::PropertyFlag::DragFloat);
			ImGuiHelper::property("Extend", size, 0, 0, ImGuiHelper::PropertyFlag::DragFloat);
			ImGui::Separator();

			ImGui::Columns(1);

			if (ImGui::Button("Edit Box"))
			{
				auto editor = static_cast<maple::Editor *>(Application::get());
				editor->editBox();
			}

			ImGui::Separator();
		}
		ImGui::Columns(1);
	}

	template <>
	inline auto ComponentEditorWidget<component::BloomData>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &bloom = reg.get<component::BloomData>(e);

		ImGui::Columns(2);
		ImGui::Separator();
		ImGuiHelper::property("Enable", bloom.enable);
		ImGuiHelper::property("Blur Scale", bloom.blurScale, 0, 1.f, maple::ImGuiHelper::PropertyFlag::DragFloat, "%.4f", 0.001);
		ImGuiHelper::property("Blur Strength", bloom.blurStrength, 1, 5);
		ImGui::Columns(1);
	}

	template <>
	inline auto ComponentEditorWidget<component::SkyboxData>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &skyData = reg.get<component::SkyboxData>(e);

		ImGui::Columns(2);
		ImGui::Separator();
		ImGuiHelper::property("CubeMap Level", skyData.cubeMapLevel, 0, 4);

		if (auto id = ImGuiHelper::combox("Cube Map", SkyboxId::Names, SkyboxId::Length, skyData.cubeMapMode); id != -1)
		{
			skyData.cubeMapMode = id;
		}
		ImGui::Columns(1);
	}

	template <>
	inline auto ComponentEditorWidget<component::Hierarchy>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto editor = static_cast<maple::Editor *>(Application::get());

		auto &data = reg.get<component::Hierarchy>(e);

		auto getName = [&](entt::entity entity) -> std::string {
			if (entity == entt::null)
			{
				return "";
			}
			return reg.get<component::NameComponent>(entity).name;
		};

		ImGui::Columns(2);
		ImGui::Separator();

		ImGuiHelper::hyperLink("Prev", getName(data.prev), " Entity id:" + std::to_string((uint32_t) data.prev), [&]() {
			editor->setSelected(data.prev);
		});

		ImGuiHelper::hyperLink("Next", getName(data.next), " Entity id:" + std::to_string((uint32_t) data.next), [&]() {
			editor->setSelected(data.next);
		});

		ImGuiHelper::hyperLink("First", getName(data.first), " Entity id:" + std::to_string((uint32_t) data.first), [&]() {
			editor->setSelected(data.first);
		});

		ImGuiHelper::hyperLink("Parent", getName(data.parent), " Entity id:" + std::to_string((uint32_t) data.parent), [&]() {
			editor->setSelected(data.parent);
		});

		ImGui::Columns(1);
	}

	template <>
	inline auto ComponentEditorWidget<component::Animator>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &animator = reg.get<component::Animator>(e);

		ImGui::Columns(2);
		ImGui::Separator();

		std::string label = animator.animation ? animator.animation->getPath() : "";

		ImGuiHelper::property("File", label, true);
		ImGuiHelper::acceptFile([&](const std::string &file) {
			auto ext = StringUtils::getExtension(file);
			if (Application::getAssetsLoaderFactory()->getSupportExtensions().count(ext) >= 1)
			{
				std::vector<std::shared_ptr<IResource>> outRes;
				Loader::load(file, outRes);
				for (auto res : outRes)
				{
					if (res->getResourceType() == FileType::Animation)
					{
						animator.animation = std::static_pointer_cast<Animation>(res);
					}
				}
			}
		});
		ImGui::Separator();
		ImGuiHelper::property("RootMotion", animator.rootMotion);

		static int32_t clip = 0;
		if (animator.animation != nullptr)
		{
			if (ImGuiHelper::property("Clip", clip, 0, animator.animation->getClipCount() - 1))
			{
				animation::play(animator, clip, 0);
			}
		}

		ImGui::Separator();
		if (animator.animation != nullptr)
		{
			if (ImGui::Button(animation::isPlaying(animator) ? "Stop" : "Play"))
			{
				if (animation::isPlaying(animator))
				{
					animation::stop(animator);
				}
				else
				{
					animation::play(animator, clip, 0);
				}
			}

			ImGui::NextColumn();

			bool disableBtn = !animation::isPlaying(animator);

			if (disableBtn)
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
			}

			if (ImGui::Button("Pause"))
			{
				animation::pause(animator);
			}

			if (disableBtn)
			{
				ImGui::PopItemFlag();
				ImGui::PopStyleVar();
			}

			ImGui::NextColumn();

			ImGui::Separator();
		}

		ImGui::Columns(1);

		if (auto clip = animation::getPlayingClip(animator); clip >= 0)
		{
			float time       = animation::getPlayingTime(animator);
			float timeLength = animator.animation->getClipLength(clip);
			//	ImGui::ProgressBar(time / timeLength);
			if (ImGui::SliderFloat("PlayingTime", &time, 0, timeLength))
			{
				animation::setPlayingTime(animator, time);
			}
		}
		ImGui::Separator();
	}

	template <>
	inline auto ComponentEditorWidget<component::FinalPass>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &data = reg.get<component::FinalPass>(e);

		ImGui::Columns(2);
		ImGui::Separator();
		ImGuiHelper::property("exposure", data.exposure, 1, 10);
		ImGuiHelper::property("toneMapIndex", data.toneMapIndex, 1, 8);
		ImGui::Columns(1);
	}

	template <>
	inline auto ComponentEditorWidget<component::BoneComponent>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &data = reg.get<component::BoneComponent>(e);

		auto &bone = data.skeleton->getBone(data.boneIndex);

		ImGui::Columns(2);
		ImGui::Separator();
		ImGuiHelper::showProperty("Bone Name", bone.name);
		ImGuiHelper::showProperty("Bone Index", std::to_string(data.boneIndex));
		ImGui::Columns(1);
	}

	template <>
	inline auto ComponentEditorWidget<ssao::component::SSAOData>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &ssao = reg.get<ssao::component::SSAOData>(e);
		ImGui::Columns(2);
		ImGui::Separator();
		ImGuiHelper::property("SSAO Enable", ssao.enable);
		ImGuiHelper::property("SSAO Radius", ssao.ssaoRadius, 0.f, 100.f);
		ImGui::Columns(1);
	}

	template <>
	inline auto ComponentEditorWidget<component::GridRender>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &data = reg.get<component::GridRender>(e);
		ImGui::Columns(2);
		ImGui::Separator();
		ImGuiHelper::property("GridRender Enable", data.enable);
		ImGui::Columns(1);
	}

	template <>
	inline auto ComponentEditorWidget<component::ShadowMapData>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &shadowMap = reg.get<component::ShadowMapData>(e);
		ImGui::Columns(2);
		ImGui::Separator();

		if (ImGuiHelper::property("Cascade Split Lambda", shadowMap.cascadeSplitLambda, 0, 1, ImGuiHelper::PropertyFlag::DragFloat, "%.4f", 0.001))
		{
			shadowMap.dirty = true;
		}

		if (ImGuiHelper::property("Initial Bias (z)", shadowMap.initialBias, 0, 1, ImGuiHelper::PropertyFlag::DragFloat, "%.4f", 0.001))
		{
			shadowMap.dirty = true;
		}

		if (auto id = ImGuiHelper::combox("ShadowMethod", ShadowingMethod::Names, ShadowingMethod::Length, shadowMap.shadowMethod); id != -1)
		{
			shadowMap.shadowMethod = static_cast<ShadowingMethod::Id>(id);
		}

		ImGui::Columns(1);
	}

	template <>
	inline auto ComponentEditorWidget<global::component::DeltaTime>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &dt = reg.get<global::component::DeltaTime>(e);
		ImGui::Columns(2);
		ImGui::Separator();
		ImGuiHelper::showProperty("Delta Time", std::to_string(dt.dt));
		ImGui::Columns(1);
	}

	template <>
	inline auto ComponentEditorWidget<component::LPVGrid>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &lpv = reg.get<component::LPVGrid>(e);
		ImGui::Columns(2);
		ImGui::Separator();
		ImGuiHelper::property("OcclusionAmplifier", lpv.occlusionAmplifier, 0.f, 100.f, maple::ImGuiHelper::PropertyFlag::DragFloat);
		ImGuiHelper::property("Propagate Count", lpv.propagateCount, 1, 16);
		ImGuiHelper::showProperty("CellSize", std::to_string(lpv.cellSize));
		ImGuiHelper::property("DebugAABB", lpv.debugAABB);
		ImGuiHelper::property("Enable", lpv.enableIndirect);
		if (lpv.debugAABB)
			ImGuiHelper::property("ShowGeometry", lpv.showGeometry);
		ImGui::Columns(1);
	}

	template <>
	inline auto ComponentEditorWidget<component::Transform>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &transform = reg.get<component::Transform>(e);

		auto rotation = glm::degrees(transform.getLocalOrientation());
		auto position = transform.getLocalPosition();
		auto scale    = transform.getLocalScale();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
		ImGui::Columns(2);
		ImGui::Separator();

		//##################################################
		if (ImGuiHelper::property("Position", position, 0, 0, ImGuiHelper::PropertyFlag::DragFloat, 0.05))
		{
			transform.setLocalPosition(position);
		}
		//##################################################
		if (ImGuiHelper::property("Rotation", rotation, 0, 0, ImGuiHelper::PropertyFlag::DragFloat, 0.5))
		{
			transform.setLocalOrientation(glm::radians(rotation));
		}
		//##################################################
		if (ImGuiHelper::property("Scale", scale, 0, 0, ImGuiHelper::PropertyFlag::DragFloat, 0.05))
		{
			transform.setLocalScale(scale);
		}
		//##################################################

		ImGui::Columns(1);
		ImGui::Separator();
		ImGui::PopStyleVar();
	}

	inline auto textureWidget(const char *label, Material *material, std::shared_ptr<Texture2D> tex, float &usingMapProperty, glm::vec4 &colorProperty, const std::function<void(const std::string &)> &callback, bool &update) -> void
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
				const bool flipImage = true;
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

		if (ImGuiHelper::property("Frame", sprite.currentFrame, 0, sprite.getFrames() - 1))
		{
		}
		if (ImGuiHelper::property("Loop", sprite.loop))
		{
		}

		auto delay = sprite.getDelay();
		if (ImGuiHelper::inputFloat("Delay", delay))
		{}

		auto timer = sprite.frameTimer;
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
	inline auto ComponentEditorWidget<component::SkinnedMeshRenderer>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &mesh = reg.get<component::SkinnedMeshRenderer>(e);

		auto &materials = mesh.mesh->getMaterial();

		ImGui::Columns(2);

		if (auto box = mesh.mesh->getBoundingBox(); box != nullptr)
		{
			ImGui::Separator();
			ImGui::Columns(1);
			ImGui::TextUnformatted("Bounds");
			ImGui::Columns(2);

			auto size   = box->size();
			auto center = box->center();

			ImGuiHelper::property("Center", center, 0, 0, ImGuiHelper::PropertyFlag::DragFloat);
			ImGuiHelper::property("Extend", size, 0, 0, ImGuiHelper::PropertyFlag::DragFloat);
			ImGui::Separator();
		}

		ImGuiHelper::property("Cast Shadow", mesh.castShadow);

		ImGui::Columns(1);
		ImGui::Separator();

		const std::string matName = "Material";
		if (materials.empty())
		{
			ImGui::TextUnformatted("Empty Material");
			if (ImGui::Button("Add Material", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
				mesh.mesh->setMaterial(std::make_shared<Material>());
		}
		else
		{
			for (auto i = 0; i < materials.size(); i++)
			{
				auto &      material = materials[i];
				std::string name     = matName + "_" + std::to_string(i);
				if (ImGui::TreeNodeEx(name.c_str(), 0))
				{
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
					{
						material->setMaterialProperites(prop);
						Application::getExecutePoint()->getGlobalComponent<global::component::MaterialChanged>().updateQueue.emplace_back(material.get());
					}

					ImGui::TreePop();
				}
			}
		}
	}

	template <>
	inline auto ComponentEditorWidget<component::MeshRenderer>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &mesh = reg.get<component::MeshRenderer>(e);

		auto &materials = mesh.mesh->getMaterial();

		ImGui::Columns(2);
		if (mesh.mesh)
			ImGuiHelper::showProperty("Mesh ID", std::to_string(mesh.mesh->getId()));

		if (auto box = mesh.mesh->getBoundingBox(); box != nullptr)
		{
			ImGui::Separator();
			ImGui::Columns(1);
			ImGui::TextUnformatted("Bounds");
			ImGui::Columns(2);

			auto size   = box->size();
			auto center = box->center();

			ImGuiHelper::property("Center", center, 0, 0, ImGuiHelper::PropertyFlag::DragFloat);
			ImGuiHelper::property("Extend", size, 0, 0, ImGuiHelper::PropertyFlag::DragFloat);
			ImGui::Separator();
		}

		ImGuiHelper::property("Cast Shadow", mesh.castShadow);

		ImGui::Columns(1);
		ImGui::Separator();

		const std::string matName = "Material";
		if (materials.empty())
		{
			ImGui::TextUnformatted("Empty Material");
			if (ImGui::Button("Add Material", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
				mesh.mesh->setMaterial(std::make_shared<Material>());
		}
		else
		{
			for (auto i = 0; i < materials.size(); i++)
			{
				auto &      material = materials[i];
				std::string name     = matName + "_" + std::to_string(i);
				if (ImGui::TreeNodeEx(name.c_str(), 0))
				{
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
					{
						material->setMaterialProperites(prop);
						Application::getExecutePoint()->getGlobalComponent<global::component::MaterialChanged>().updateQueue.emplace_back(material.get());
					}

					ImGui::TreePop();
				}
			}
		}
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

		if (ImGuiHelper::property("Intensity", light.lightData.intensity, 0.0f, 100.0f))
		{        //update once
			auto &transform = reg.get<component::Transform>(e);
			transform.setLocalPosition(transform.getLocalPosition());
		}

		if (static_cast<component::LightType>(light.lightData.type) == component::LightType::SpotLight)
			ImGuiHelper::property("Angle", light.lightData.angle, -1.0f, 1.0f);

		ImGuiHelper::property("Show Frustum", light.showFrustum);
		if (ImGuiHelper::property("Cast Shadow", light.castShadow))
		{
			auto &transform = reg.get<component::Transform>(e);
			transform.setLocalPosition(transform.getLocalPosition());
		}

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
						mono::addScript(mono, entry.path().string(), (int32_t) e);
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
		if (ImGui::BeginCombo("", !env.pseudoSky ? "Environment" : "PseudoSky", 0))
		{
			for (auto n = 0; n < 2; n++)
			{
				if (ImGui::Selectable(boxItems[n], true))
				{
					env.pseudoSky = n == 1;
				}
			}
			ImGui::EndCombo();
		}

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		if (!env.pseudoSky)
		{
			auto label = env.filePath;

			auto updated = ImGuiHelper::property("File", label, true);
			ImGuiHelper::acceptFile([&](const std::string &file) {
				if (StringUtils::isTextureFile(file))
				{
					environment::init(env, file);
				}
			});

			if (updated)
			{
				ImGui::Columns(1);
				ImGui::Separator();

				if (env.equirectangularMap)
				{
					ImGuiHelper::image(env.equirectangularMap.get(), {64, 64});
				}
			}
		}
		else
		{
			ImGuiHelper::property("SkyColor Top", env.skyColorTop, ImGuiHelper::PropertyFlag::ColorProperty);
			ImGuiHelper::property("SkyColor Bottom", env.skyColorBottom, ImGuiHelper::PropertyFlag::ColorProperty);
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
		std::string                      currentController = maple::camera_controller::typeToString(controllerComp.type);
		if (ImGui::BeginCombo("", currentController.c_str(), 0))        // The second parameter is the label previewed before opening the combo.
		{
			for (auto n = 0; n < controllerTypes.size(); n++)
			{
				bool isSelected = currentController == controllerTypes[n];
				if (ImGui::Selectable(controllerTypes[n].c_str(), currentController.c_str()))
				{
					camera_controller::setControllerType(controllerComp, maple::camera_controller::stringToType(controllerTypes[n]));
				}
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		if (controllerComp.cameraController)
			controllerComp.cameraController->onImGui();

		ImGui::Columns(1);
		ImGui::Separator();
		ImGui::PopStyleVar();
	}

	template <>
	inline auto ComponentEditorWidget<component::Atmosphere>(entt::registry &reg, entt::registry::entity_type e) -> void
	{
		auto &atmosphere = reg.get<component::Atmosphere>(e);

		ImGui::Columns(2);

		ImGuiHelper::property("Render To Screen", atmosphere.renderToScreen);

		ImGui::Columns(3);
		ImGui::Separator();

		ImGuiHelper::propertyWithDefault("Surface Radius", atmosphere.surfaceRadius, 1.f, 1.f, component::Atmosphere::SURFACE_RADIUS, ImGuiHelper::PropertyFlag::DragFloat);
		ImGuiHelper::propertyWithDefault("Atmosphere Radius", atmosphere.atmosphereRadius, 1.f, 1.f, component::Atmosphere::ATMOSPHE_RERADIUS, ImGuiHelper::PropertyFlag::DragFloat);

		ImGuiHelper::propertyWithDefault("Center Point", atmosphere.centerPoint, -10000.f, 0.f, {0.f, -component::Atmosphere::SURFACE_RADIUS, 0.f}, ImGuiHelper::PropertyFlag::DragFloat);
		auto mie = 10000.f * atmosphere.mieScattering;

		if (ImGuiHelper::propertyWithDefault("Mie Scattering", mie, 0.f, 10.f, component::Atmosphere::MIE_SCATTERING * 10000.f, ImGuiHelper::PropertyFlag::DragFloat, 0.01))
		{
			atmosphere.mieScattering = mie / 10000.f;
		}
		auto ray = 10000.f * atmosphere.rayleighScattering;

		if (ImGuiHelper::propertyWithDefault("Ray Scattering", ray, 0.f, 10.f, component::Atmosphere::RAYLEIGH_SCATTERING * 10000.f, ImGuiHelper::PropertyFlag::DragFloat, 0.01))
		{
			atmosphere.rayleighScattering = ray / 10000.f;
		}
		ImGuiHelper::propertyWithDefault("Anisotropy(g)", atmosphere.g, 0.f, 1.f, component::Atmosphere::G, ImGuiHelper::PropertyFlag::DragFloat, 0.01);

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
		auto &registry         = Application::getExecutePoint()->getRegistry();
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
		auto &editor = static_cast<Editor &>(*Application::get());
		registerInspector(enttEditor);
		MM::EntityEditor<entt::entity>::ComponentInfo info;
		info.hasChildren  = true;
		info.childrenDraw = [](entt::registry &registry, entt::entity ent) {
			auto mono = registry.try_get<component::MonoComponent>(ent);

			if (mono)
			{
				for (auto &script : mono->scripts)
				{
					ImGui::PushID(script.first.c_str());
					if (ImGui::Button("-"))
					{
						mono::remove(*mono, script.first);
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
				mono::addScript(mono, fileName, (int32_t) ent);
			}
			else if (StringUtils::isLuaFile(fileName))
			{
				auto &lua = registry.get_or_emplace<component::LuaComponent>(ent);
			}
		};
	}

	auto PropertiesWindow::drawResource(const std::string &path) -> void
	{
		ImGui::TextUnformatted(StringUtils::getFileName(path).c_str());

		ImGui::Separator();

		if (File::isKindOf(path, FileType::Material))
		{
			/*	auto material = Application::getCache()->emplace<Material>(path);

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
			ImGui::PopStyleVar();*/
		}
	}

};        // namespace maple
