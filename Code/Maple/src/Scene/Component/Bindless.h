//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <glm/glm.hpp>
#include <unordered_map>

namespace maple
{
	namespace global::component
	{
		struct Bindless
		{
			std::unordered_map<uint32_t, uint32_t> meshIndices;
			std::unordered_map<uint32_t, uint32_t> materialIndices;
		};
	}        // namespace global::component
}        // namespace maple