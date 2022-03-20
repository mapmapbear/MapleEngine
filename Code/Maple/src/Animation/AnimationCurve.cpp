//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "AnimationCurve.h"
#include "Math/MathUtils.h"

namespace maple
{
    auto AnimationCurve::linear(float timeStart, float valueStart, float timeEnd, float valueEnd) -> AnimationCurve
    {
		AnimationCurve curve;
		float tangent = (valueEnd - valueStart) / (timeEnd - timeStart);
		curve.addKey(timeStart, valueEnd, tangent, tangent);
		curve.addKey(timeEnd, valueEnd, tangent, tangent);
		return curve;
    }

    auto AnimationCurve::addKey(float time, float value, float inTangent, float outTangent) -> void
    {
		keys.push_back({ time, value, inTangent, outTangent });
    }

    auto AnimationCurve::evaluate(float time) const -> float
    {
		if (keys.empty())
		{
			return 0;
		}

		const auto& back = keys.back();
		if (time >= back.time)
		{
			return back.value;
		}

		for (int i = 0; i < keys.size(); ++i)
		{
			const auto& key = keys[i];

			if (time < key.time)
			{
				if (i == 0)
				{
					return key.value;
				}
				else
				{
					return evaluate(time, keys[i - 1], key);
				}
			}
		}

		return 0;
    }

    float AnimationCurve::evaluate(float time, const Key& k0, const Key& k1)
    {
		//result.Time = k0.time + (k1.time - k0.time) * alpha;
		float dt = std::abs((time - k0.time) / (k1.time - k0.time));
		return MathUtils::lerp(k0.value, k1.value, dt, false);
    }
};