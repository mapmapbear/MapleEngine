//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Math/BoundingBox.h"
#include <glm/glm.hpp>

#include <IconsMaterialDesignIcons.h>

namespace maple
{
	namespace component 
	{
		struct BoundingBoxComponent
		{
			constexpr static char* ICON = ICON_MDI_BOX;

			BoundingBox* box;
		};
	}
};        // namespace maple
