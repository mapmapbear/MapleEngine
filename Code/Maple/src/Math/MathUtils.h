//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <algorithm>
#include <glm/glm.hpp>
#include <string>

namespace maple
{
	namespace MathUtils
	{
		constexpr float EPS = 0.000001f;

		inline float lerp(float from, float to, float t, bool clamp01 = true)
		{
			if (clamp01)
			{
				t = std::min<float>(std::max<float>(t, 0), 1);
			}
			return from + (to - from) * t;
		}

		inline auto lerp(const glm::vec3 &from, const glm::vec3 &to, float t)
		{
			return from * (1.0f - t) + to * t;
		}

		// vec4 comparators
		inline bool operator>=(const glm::vec4 &left, const glm::vec4 &other)
		{
			return left.x >= other.x && left.y >= other.y && left.z >= other.z && left.w >= other.w;
		}

		inline bool operator<=(const glm::vec4 &left, const glm::vec4 &other)
		{
			return left.x <= other.x && left.y <= other.y && left.z <= other.z && left.w <= other.w;
			;
		}

		// vec3 comparators
		inline bool operator>=(const glm::vec3 &left, const glm::vec3 &other)
		{
			return left.x >= other.x && left.y >= other.y && left.z >= other.z;
		}

		inline bool operator<=(const glm::vec3 &left, const glm::vec3 &other)
		{
			return left.x <= other.x && left.y <= other.y && left.z <= other.z;
		}

		// vec3 comparators
		inline bool operator>=(const glm::vec2 &left, const glm::vec2 &other)
		{
			return left.x >= other.x && left.y >= other.y;
		}

		inline bool operator<=(const glm::vec2 &left, const glm::vec2 &other)
		{
			return left.x <= other.x && left.y <= other.y;
		}

		inline auto worldToScreen(const glm::vec3 &worldPos, const glm::mat4 &mvp, float width, float height, float winPosX = 0.0f, float winPosY = 0.0f)
		{
			glm::vec4 trans = mvp * glm::vec4(worldPos, 1.0f);
			trans *= 0.5f / trans.w;
			trans += glm::vec4(0.5f, 0.5f, 0.0f, 0.0f);
			trans.y = 1.f - trans.y;
			trans.x *= width;
			trans.y *= height;
			//trans.x += winPosX;
			//trans.y += winPosY;
			return glm::vec2(trans.x, trans.y);
		}

		//only support for float value and vector
		template <class T>
		typename std::enable_if<
		    std::is_floating_point<T>::value ||
		        std::is_same<T, glm::vec2>::value ||
		        std::is_same<T, glm::vec3>::value ||
		        std::is_same<T, glm::vec4>::value,
		    bool>::type
		    equals(const T &lhs, const T &rhs)
		{
			T eps(0.000001f);
			return lhs + eps >= rhs && lhs - eps <= rhs;
		}
	};        // namespace MathUtils
};            // namespace maple
