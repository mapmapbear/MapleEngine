#pragma once

#include <IconsMaterialDesignIcons.h>
#include <font_awesome_5.h>

#include "Engine/Camera.h"
#include "Scene/Component/Light.h"
#include "Scene/Component/Transform.h"
#include "Scene/Component/Atmosphere.h"
#include "Scene/Component/BoundingBox.h"
#include "Scene/Component/CameraControllerComponent.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Component/Transform.h"
#include "Scene/Component/VolumetricCloud.h"
#include "Scene/Component/Environment.h"
#include "Engine/Renderer/AtmosphereRenderer.h"
#include "Engine/Renderer/GridRenderer.h"
#include "Engine/Renderer/PostProcessRenderer.h"
#include "Engine/Renderer/SkyboxRenderer.h"
#include "Engine/Renderer/FinalPass.h"
#include "Engine/Renderer/ShadowRenderer.h"
#include "Engine/LPVGI/ReflectiveShadowMap.h"
#include "Engine/LPVGI/LightPropagationVolume.h"
#include "Engine/VXGI/DrawVoxel.h"
#include "Engine/VXGI/Voxelization.h"

#include "Scripts/Lua/LuaComponent.h"
#include "Scripts/Mono/MonoComponent.h"
#include "Animation/Animator.h"
#include "2d/Sprite.h"
#include "Physics/RigidBody.h"
#include "Physics/Collider.h"

namespace maple
{
	template <typename T>
	constexpr char* ICON = "";
	template <typename T>
	constexpr char* ICON< const T > = ICON< T >;
}

#define COMP_ICON(Comp, Icon) \
namespace maple	\
{	\
	template<> \
	inline constexpr char* ICON< Comp > = Icon; \
}

COMP_ICON(Camera,								ICON_MDI_CAMERA);
COMP_ICON(component::Light,						ICON_MDI_LIGHTBULB);
COMP_ICON(component::DeltaTime,					ICON_MDI_TIMELAPSE);
COMP_ICON(component::Hierarchy,					ICON_MDI_TREE);
COMP_ICON(component::Environment,				ICON_MDI_EARTH);
COMP_ICON(component::Transform,					ICON_MDI_VECTOR_LINE);
COMP_ICON(component::Atmosphere,				ICON_MDI_WEATHER_SUNNY);
COMP_ICON(component::BoneComponent,				ICON_MDI_BONE);
COMP_ICON(component::BoundingBoxComponent,		ICON_MDI_SHAPE);
COMP_ICON(component::SkinnedMeshRenderer,		ICON_MDI_HUMAN);
COMP_ICON(component::MeshRenderer,				ICON_MDI_CUBE);
COMP_ICON(component::VolumetricCloud,			ICON_MDI_WEATHER_CLOUDY);
COMP_ICON(component::ReflectiveShadowData,		ICON_MDI_BOX_SHADOW);
COMP_ICON(component::LPVGrid,					ICON_MDI_GRID_LARGE);
COMP_ICON(component::ShadowMapData,				ICON_MDI_BOX_SHADOW);
COMP_ICON(component::CameraControllerComponent, ICON_MDI_CONTROLLER_CLASSIC);
COMP_ICON(component::FinalPass,					ICON_MDI_SCREEN_ROTATION);
COMP_ICON(component::GridRender,				ICON_MDI_GRID);
COMP_ICON(component::SSAOData,					ICON_MDI_BOX_SHADOW);
COMP_ICON(component::SkyboxData,				ICON_MDI_HEART_CIRCLE);
COMP_ICON(component::Animator,					ICON_MDI_ANIMATION);
COMP_ICON(component::Sprite,					ICON_MDI_IMAGE);
COMP_ICON(component::AnimatedSprite,			ICON_MDI_IMAGE_AREA);
COMP_ICON(component::LuaComponent,				ICON_MDI_LANGUAGE_LUA);
COMP_ICON(component::MonoComponent,				ICON_MDI_LANGUAGE_CSHARP);
COMP_ICON(component::BloomData,					ICON_MDI_BRIGHTNESS_AUTO);
COMP_ICON(physics::component::RigidBody,		ICON_MDI_NATURE_PEOPLE);
COMP_ICON(physics::component::Collider,			ICON_MDI_BOOMBOX);

COMP_ICON(vxgi_debug::global::component::DrawVoxelRender, ICON_MDI_LIGHTHOUSE_ON);
COMP_ICON(component::Voxelization, ICON_MDI_LIGHTHOUSE_ON);




