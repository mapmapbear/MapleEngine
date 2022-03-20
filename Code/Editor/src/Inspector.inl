//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Vientiane/ReflectiveShadowMap.h"
#include "Engine/Vientiane/LightPropagationVolume.h"
#include "Engine/Renderer/PostProcessRenderer.h"
#include "Scene/Component/BoundingBox.h"


namespace maple
{
#define TRIVIAL_COMPONENT(ComponentType, show, showName)         \
	{                                                            \
		std::string name = ComponentType::ICON;                  \
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
		TRIVIAL_COMPONENT(component::SkinnedMeshRenderer, false, "Skinned Mesh Renderer");
		TRIVIAL_COMPONENT(component::Atmosphere, true, "Atmosphere");
		TRIVIAL_COMPONENT(component::VolumetricCloud, true, "Volumetric Cloud");
		TRIVIAL_COMPONENT(component::LightProbe, true, "Light Probe");
		TRIVIAL_COMPONENT(component::LPVGrid, false, "LPV Grid");
		TRIVIAL_COMPONENT(component::ReflectiveShadowData, false, "Reflective Shadow Map");
		TRIVIAL_COMPONENT(component::ShadowMapData, false, "Shadow Map");
		TRIVIAL_COMPONENT(component::BoundingBoxComponent, false, "BoundingBox");
		TRIVIAL_COMPONENT(component::SSAOData, false, "SSAO Data");
		TRIVIAL_COMPONENT(component::DeltaTime, false, "Delta Time");
		TRIVIAL_COMPONENT(component::GridRender, false, "Grid Render");
	}
}