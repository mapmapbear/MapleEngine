#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#include "Common.glsl"

layout (set = 0, binding = 4) uniform samplerCube uSkybox;

rayPayloadInEXT PathTracePayload inPayload;

void main()
{
    vec3 envSample = texture(uSkybox, gl_WorldRayDirectionEXT).rgb; 
    if (inPayload.depth == 0)
        inPayload.L = envSample;
    else
        inPayload.L = inPayload.T * envSample;
}
