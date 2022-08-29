
//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "DescriptorSet.h"

#ifdef MAPLE_VULKAN
#	include "RHI/Vulkan/VulkanDescriptorSet.h"
#endif        // MAPLE_VULKAN

#ifdef MAPLE_OPENGL
#	include "RHI/OpenGL/GLDescriptorSet.h"
#endif

namespace maple
{
	std::vector<std::shared_ptr<maple::DescriptorSet>> DescriptorSet::setCache;

	namespace
	{
		inline auto isSubLayout(const DescriptorLayoutInfo &desc, const DescriptorLayoutInfo &parent)
		{
			
			return true;
		}
	}        // namespace

	auto DescriptorSet::createWithLayout(const DescriptorLayoutInfo &desc) -> std::shared_ptr<DescriptorSet>
	{
		for (auto &set : setCache)
		{
			if (isSubLayout(desc, set->getDescriptorLayoutInfo()))
			{
				return set;
			}
		}

#ifdef MAPLE_VULKAN
		return std::make_shared<VulkanDescriptorSet>(desc);
#endif

#ifdef MAPLE_OPENGL
		return std::make_shared<GLDescriptorSet>(desc);
#endif
	}

}        // namespace maple
