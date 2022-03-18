//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Engine/Core.h"
#include <glm/glm.hpp>
#include <imgui.h>
#include <string>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

namespace maple
{
	class Texture;
	class Quad2D;

	namespace ImGuiHelper
	{
		enum class PropertyFlag
		{
			None          = 0,
			ColorProperty = 1,
			InputFloat    = 2
		};

		MAPLE_EXPORT auto tooltip(const char *str) -> void;
		MAPLE_EXPORT auto property(const std::string& name, bool& value) -> bool;
		MAPLE_EXPORT auto showProperty(const std::string& name, float value) -> bool;
		MAPLE_EXPORT auto property(const std::string &name, std::string &value, bool disable = false) -> bool;

		MAPLE_EXPORT auto property(const std::string &name, float &value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None, const char *format = "%.3f", float speed = 1.f) -> bool;
		MAPLE_EXPORT auto propertyWithDefault(const std::string &name, float &value, float min = -1.0f, float max = 1.0f, float defaultValue = 0.f, PropertyFlag flags = PropertyFlag::None, float speed = 1.f) -> bool;
		MAPLE_EXPORT auto propertyWithDefault(const std::string &name, glm::vec3 &value, float min = -1.0f, float max = 1.0f, const glm::vec3 &defaultValue = {}, PropertyFlag flags = PropertyFlag::None, float speed = 1.f) -> bool;

		MAPLE_EXPORT auto property(const std::string &name, int32_t &value, int32_t min = -1, int32_t max = 1, PropertyFlag flags = PropertyFlag::None) -> bool;
		MAPLE_EXPORT auto property(const std::string &name, uint32_t &value, uint32_t min = 0, uint32_t max = 1, PropertyFlag flags = PropertyFlag::None) -> bool;
		MAPLE_EXPORT auto property(const std::string &name, glm::vec2 &value, PropertyFlag flags) -> bool;
		MAPLE_EXPORT auto property(const std::string &name, glm::vec2 &value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None) -> bool;
		MAPLE_EXPORT auto property(const std::string &name, glm::vec3 &value, PropertyFlag flags) -> bool;
		MAPLE_EXPORT auto property(const std::string &name, glm::vec3 &value, float min = -1.0f, float max = 1.0f, PropertyFlag flags = PropertyFlag::None, float speed = 1.f) -> bool;
		MAPLE_EXPORT auto property(const std::string &name, glm::vec4 &value, bool exposeW, PropertyFlag flags) -> bool;
		MAPLE_EXPORT auto property(const std::string &name, glm::vec4 &value, float min = -1.0f, float max = 1.0f, bool exposeW = false, PropertyFlag flags = PropertyFlag::None) -> bool;

		MAPLE_EXPORT auto inputFloat(const std::string &name, float &value, float min = -1.0f, float max = 1.0f) -> bool;
		MAPLE_EXPORT auto image(const Texture *texture, const glm::vec2 &size) -> void;
		MAPLE_EXPORT auto imageButton(const Quad2D * texture,const glm::vec2 & scale) -> bool;
	};        // namespace ImGuiHelper
};            // namespace maple
