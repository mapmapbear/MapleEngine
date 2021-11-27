//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstddef>
#include <cstdint>

namespace Maple
{
	namespace HashCode
	{
		inline auto hashCode(std::size_t &seed) -> void
		{}

		template <typename T, typename... Rest>
		inline auto hashCode(std::size_t &seed, const T &v, Rest... rest) -> void
		{
			std::hash<T> hasher;
			seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			hashCode(seed, rest...);
		}

		inline auto hashCode(const char *str) -> uint32_t
		{
			const char *c   = str;
			uint32_t    ret = 0x00000000;
			while (*c)
			{
				ret = (ret << 5) - ret + (*c == '/' ? '\\' : *c);
				c++;
			}
			return ret ? ret : 0xffffffff;
		}
	};        // namespace HashCode
};            // namespace Maple
