//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Engine/IconsDefine.inl"


namespace maple
{
#define TRIVIAL_COMPONENT(ComponentType, show, showName)         \
	{                                                            \
		std::string name = ICON<ComponentType>;                  \
		name += "\t";                                            \
		name += showName;;                                       \
		enttEditor.registerComponent<ComponentType>(name, show); \
	}

	inline auto registerInspector(MM::EntityEditor<entt::entity> & enttEditor)
	{
		TRIVIAL_COMPONENT(Camera, true, "Camera");
		TRIVIAL_COMPONENT(component::Transform, true, "Transform");
		TRIVIAL_COMPONENT(component::Light, true, "Light");
		TRIVIAL_COMPONENT(component::CameraControllerComponent, true, "Camera Controller");
		TRIVIAL_COMPONENT(component::Environment, true, "Environment");
		TRIVIAL_COMPONENT(component::Sprite, true, "Sprite");
		TRIVIAL_COMPONENT(component::AnimatedSprite, true, "Animation Sprite");
		TRIVIAL_COMPONENT(component::MeshRenderer, false, "Mesh Renderer");
		TRIVIAL_COMPONENT(component::SkinnedMeshRenderer, false, "Skinned Mesh");
		TRIVIAL_COMPONENT(component::Atmosphere, true, "Atmosphere");
		TRIVIAL_COMPONENT(component::VolumetricCloud, true, "Volumetric Cloud");
		//TRIVIAL_COMPONENT(component::LightProbe, true, "Light Probe");
		TRIVIAL_COMPONENT(component::LPVGrid, false, "LPV Grid");
		TRIVIAL_COMPONENT(component::ReflectiveShadowData, false, "Reflective Shadow Map");
		TRIVIAL_COMPONENT(component::ShadowMapData, false, "Shadow Map");
		TRIVIAL_COMPONENT(component::BoundingBoxComponent, false, "BoundingBox");
		TRIVIAL_COMPONENT(component::SSAOData, false, "SSAO Data");
		TRIVIAL_COMPONENT(component::DeltaTime, false, "Delta Time");
		TRIVIAL_COMPONENT(component::GridRender, false, "Grid Render");
		TRIVIAL_COMPONENT(component::BoneComponent, false, "Bone");
		TRIVIAL_COMPONENT(component::Animator, true, "Animator");
		TRIVIAL_COMPONENT(component::FinalPass, false, "FinalPass");
		TRIVIAL_COMPONENT(component::SkyboxData, false, "Skybox");
		TRIVIAL_COMPONENT(component::Hierarchy, false, "Hierarchy");
		TRIVIAL_COMPONENT(component::BloomData, false, "Bloom");
		TRIVIAL_COMPONENT(physics::component::RigidBody, true, "RigidBody");
		TRIVIAL_COMPONENT(physics::component::Collider, true, "Collider");
		TRIVIAL_COMPONENT(vxgi_debug::global::component::DrawVoxelRender, false, "VXGI-Debug");
		TRIVIAL_COMPONENT(component::Voxelization, false, "VXGI-Component");
	}
}