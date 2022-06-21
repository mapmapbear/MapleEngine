//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Engine/Core.h"
#include "Math/BoundingBox.h"
#include "Scene/System/ExecutePoint.h"
#include <glm/glm.hpp>

namespace maple
{
	namespace ddgi::component
	{
		struct IrradianceVolume
		{
			BoundingBox aabb;
		};
	};        // namespace ddgi::component
}        // namespace maple