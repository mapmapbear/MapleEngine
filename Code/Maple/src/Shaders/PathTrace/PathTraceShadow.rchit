#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

rayPayloadInEXT float visibility;

void main()
{
    visibility = 0.0;
}

