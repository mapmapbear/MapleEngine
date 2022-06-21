//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "CameraControllerComponent.h"
#include "Engine/CameraController.h"
#include "Others/Console.h"

namespace maple
{
	auto camera_controller::setControllerType(component::CameraControllerComponent &controller, maple::ControllerType type) -> void
	{
		controller.type = type;
		switch (type)
		{
			case ControllerType::FPS:
				LOGW("{0} does not implement", __FUNCTION__);
				break;
			case ControllerType::EditorCamera:
				controller.cameraController = std::make_shared<EditorCameraController>();
				break;
		}
	}
};        // namespace maple
