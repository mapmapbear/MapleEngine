//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include <functional>
#include <memory>
#include <renderdoc_app.h>
#include <stdarg.h>
#include <string>
#include <vector>

namespace maple
{
	class MAPLE_EXPORT RenderDocExt
	{
	  public:
		RenderDocExt() = default;

		NO_COPYABLE(RenderDocExt);

		auto openLib() -> void;

		auto startCapture() -> void;

		auto endCapture() -> void;

		inline auto isEnabled() const
		{
			return enable;
		}

		inline auto toggleEnable()
		{
			enable = !enable;
		}

	  private:
		bool                 enable = false;
		RENDERDOC_API_1_5_0 *doc    = nullptr;
	};        // namespace StringUtils
};            // namespace maple
