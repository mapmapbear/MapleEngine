//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Engine/Core.h"
#include <memory>
#include <string>

namespace maple
{
	class CameraController;

	enum class ControllerType : int32_t
	{
		FPS = 0,
		EditorCamera,
		Custom
	};

	namespace component
	{
		struct CameraControllerComponent
		{
			ControllerType                    type = ControllerType::FPS;
			std::shared_ptr<CameraController> cameraController;
		};
	}        // namespace component

	namespace camera_controller
	{
		inline auto typeToString(ControllerType type)
		{
			switch (type)
			{
				case ControllerType::FPS:
					return "FPS";
				case ControllerType::EditorCamera:
					return "Editor";
			}
			return "Custom";
		}

		inline auto stringToType(const std::string &type)
		{
			if (type == "FPS")
				return ControllerType::FPS;
			if (type == "Editor")
				return ControllerType::EditorCamera;
			return ControllerType::Custom;
		}

		auto MAPLE_EXPORT setControllerType(component::CameraControllerComponent &controller, maple::ControllerType type) -> void;
	}        // namespace camera_controller
};           // namespace maple
