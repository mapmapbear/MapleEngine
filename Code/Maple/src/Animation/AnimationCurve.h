//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>
#include "Engine/Core.h"

namespace maple
{
	class MAPLE_EXPORT AnimationCurve
	{
	private:
		struct Key
		{
			float time;
			float value;
			float inTangent;
			float outTangent;
		};

	public:
		static auto linear(float timeStart, float valueStart, float timeEnd, float valueEnd) ->AnimationCurve;
		auto addKey(float time, float value, float inTangent, float outTangent) -> void;
		auto evaluate(float time) const -> float;

	private:
		static auto evaluate(float time, const Key& k0, const Key& k1) -> float;

	private:
		std::vector<Key> keys;
	};
};