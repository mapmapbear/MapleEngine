//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "RHI/StorageBuffer.h"

namespace maple
{
	class GLShader;

	class GLStorageBuffer : public StorageBuffer
	{
	  public:
		GLStorageBuffer();
		GLStorageBuffer(uint32_t size, const void *data);
		~GLStorageBuffer();
		auto setData(uint32_t size, const void *data) -> void override;
		auto bind(uint32_t slot) const -> void;
		auto unbind() const -> void;

	  private:
		uint32_t handle{};
		uint32_t size = 0;
	};
}        // namespace maple
