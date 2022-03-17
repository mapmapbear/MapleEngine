
//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "DescriptorSet.h"

namespace maple
{
	static bool updateValue = true;

	auto DescriptorSet::canUpdate() ->bool
	{
		return updateValue;
	}

	auto DescriptorSet::toggleUpdate(bool u) -> void
	{
		updateValue = u;
	}
}
