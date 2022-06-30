//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <cstdint>
#include <functional>
#include <memory>

namespace maple
{
	enum class MemoryUsage
	{
		MEMORY_USAGE_UNKNOWN              = 0,
		MEMORY_USAGE_GPU_ONLY             = 1,
		MEMORY_USAGE_CPU_ONLY             = 2,
		MEMORY_USAGE_CPU_TO_GPU           = 3,
		MEMORY_USAGE_GPU_TO_CPU           = 4,
		MEMORY_USAGE_CPU_COPY             = 5,
		MEMORY_USAGE_GPU_LAZILY_ALLOCATED = 6,
	};

	struct BufferOptions
	{
		bool     indirect;
		uint32_t vmaUsage;
		uint32_t vmaCreateFlags;

		constexpr BufferOptions() :
		    indirect(false),
		    vmaUsage(0),
		    vmaCreateFlags(0)
		{}
		constexpr BufferOptions(bool indirect, uint32_t vamUsage = 0, uint32_t vmaCreateFlags = 0) :
		    indirect(indirect),
		    vmaUsage(vamUsage),
		    vmaCreateFlags(vmaCreateFlags)
		{}
	};

	class StorageBuffer
	{
	  public:
		using Ptr                = std::shared_ptr<StorageBuffer>;
		virtual ~StorageBuffer() = default;
		static auto create(const BufferOptions &options = {}) -> std::shared_ptr<StorageBuffer>;
		static auto create(uint32_t size, const void *data, const BufferOptions &options = {}) -> std::shared_ptr<StorageBuffer>;
		static auto create(uint32_t size, uint32_t flags, const BufferOptions &options = {}) -> std::shared_ptr<StorageBuffer>;

		virtual auto setData(uint32_t size, const void *data) -> void           = 0;
		virtual auto mapMemory(const std::function<void(void *)> &call) -> void = 0;
		virtual auto unmap() -> void                                            = 0;
		virtual auto map() -> void *                                            = 0;
		virtual auto getDeviceAddress() const -> uint64_t                       = 0;
	};
}        // namespace maple
