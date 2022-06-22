//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include <memory>

namespace maple
{
	class StorageBuffer;

	class AccelerationStructure
	{
	  public:
		using Ptr = std::shared_ptr<AccelerationStructure>;

		static auto createTopLevel(uint64_t address, uint32_t instancesCount) -> Ptr;

		virtual auto getBuildScratchSize() const -> uint64_t = 0;
	};
}        // namespace maple