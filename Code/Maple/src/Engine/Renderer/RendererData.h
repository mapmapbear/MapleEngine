//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <glm/glm.hpp>
#include "Math/Frustum.h"
namespace maple
{
	namespace component
	{
		struct CameraView
		{
			glm::mat4 proj;
			glm::mat4 view;
			glm::mat4 projView;
			glm::mat4 projViewOld;
			float     nearPlane;
			float     farPlane;
			Frustum   frustum;
		};
	}
}