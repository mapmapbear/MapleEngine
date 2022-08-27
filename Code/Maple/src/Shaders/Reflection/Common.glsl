#ifndef REFLECTION_COMMON
#define REFLECTION_COMMON

#define MIRROR_REFLECTIONS_ROUGHNESS_THRESHOLD 0.05f
#define DDGI_REFLECTIONS_ROUGHNESS_THRESHOLD 0.75f

struct ReflectionPayload
{
    vec3 color;
    float rayLength;
};

#endif