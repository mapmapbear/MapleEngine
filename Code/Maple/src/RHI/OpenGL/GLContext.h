//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "RHI/GraphicsContext.h"

namespace maple
{
	class MAPLE_EXPORT GLContext : public GraphicsContext
	{
	  public:
		GLContext();
		~GLContext();

		auto present() -> void override;
		auto onImGui() -> void override;

		auto waitIdle() const -> void override{};
		auto init() -> void override;

		inline auto getGPUMemoryUsed() -> float override
		{
			return 0.f;
		};

		inline auto getTotalGPUMemory() -> float override
		{
			return 0.f;
		};

		inline auto getMinUniformBufferOffsetAlignment() const -> size_t override
		{
			return 256;
		}

		inline auto alignedDynamicUboSize(size_t size) const->size_t override
		{
			//todo..
			return size;
		}
	};
}        // namespace maple
