//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include <string>

namespace maple
{
	class MAPLE_EXPORT ImNotification
	{
	  public:
		enum class Type
		{
			None,
			Success,
			Warning,
			Error,
			Info
		};

		static auto onImGui() -> void;
		static auto makeNotification(const std::string &title, const std::string &str, const Type type) -> void;
	};
};        // namespace maple
