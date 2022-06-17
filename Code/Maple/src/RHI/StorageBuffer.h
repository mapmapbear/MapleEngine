//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <memory>
#include <cstdint>

namespace maple
{
	class StorageBuffer
	{
	public:
		virtual ~StorageBuffer() = default;
		static auto create() -> std::shared_ptr<StorageBuffer>;
		static auto create(uint32_t size, const void* data)	-> std::shared_ptr<StorageBuffer>;

		virtual auto setData(uint32_t size, const void* data) -> void = 0;
	};
}
