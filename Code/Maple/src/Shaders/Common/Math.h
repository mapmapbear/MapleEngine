#ifndef MATH_H
#define MATH_H

#define PI 3.1415926535897932384626433832795

const float PHI = 1.61803398874989484820459;  // Φ = Golden Ratio   

float signNotZero(in float k)
{
    return (k >= 0.0) ? 1.0 : -1.0;
}

vec2 signNotZero(in vec2 v)
{
    return vec2(signNotZero(v.x), signNotZero(v.y));
}

float square(float v)
{
    return v * v;
}

vec3 square(vec3 v)
{
    return v * v;
}

vec2 octEncode(in vec3 v) 
{
    float l1norm = abs(v.x) + abs(v.y) + abs(v.z);
    vec2 result = v.xy * (1.0 / l1norm);
    if (v.z < 0.0)
        result = (1.0 - abs(result.yx)) * signNotZero(result.xy);
    return result;
}

vec3 octDecode(vec2 o)
{
    vec3 v = vec3(o.x, o.y, 1.0 - abs(o.x) - abs(o.y));

    if (v.z < 0.0)
        v.xy = (1.0 - abs(v.yx)) * signNotZero(v.xy);

    return normalize(v);
}

#endif