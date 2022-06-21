//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <cstdint>
#include <functional>
#include <memory>

namespace maple
{
	struct BufferOptions
	{
		bool indirect;
		bool gpuOnly;
		constexpr BufferOptions() :
		    indirect(false),
		    gpuOnly(false)
		{}
		constexpr BufferOptions(bool indirect, bool gpuOnly = false) :
		    indirect(indirect),
		    gpuOnly(gpuOnly)
		{}
	};

	class StorageBuffer
	{
	  public:
		using Ptr                = std::shared_ptr<StorageBuffer>;
		virtual ~StorageBuffer() = default;
		static auto  create(const BufferOptions &options = {}) -> std::shared_ptr<StorageBuffer>;
		static auto  create(uint32_t size, const void *data, const BufferOptions &options = {}) -> std::shared_ptr<StorageBuffer>;
		virtual auto setData(uint32_t size, const void *data) -> void           = 0;
		virtual auto mapMemory(const std::function<void(void *)> &call) -> void = 0;
		virtual auto unmap() -> void                                            = 0;
		virtual auto map() -> void *                                            = 0;
	};
}        // namespace maple
