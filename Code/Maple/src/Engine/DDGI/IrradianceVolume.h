//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Scene/System/ExecutePoint.h"
#include "Math/BoundingBox.h"
#include "Engine/Core.h"
#include <glm/glm.hpp>

namespace maple
{
    namespace ddgi::component 
    {
		struct IrradianceVolume
		{
			BoundingBox aabb;
		};
    };
}