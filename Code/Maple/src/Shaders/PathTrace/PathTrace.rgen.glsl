#ifndef PATH_TRACE_RGEN
#define PATH_TRACE_RGEN

#include "../Common/Light.h"

////////////////////////Scene Infos////////////////////////////////
layout (set = 0, binding = 0, std430) readonly buffer MaterialBuffer 
{
    Material data[];
} Materials;

layout (set = 0, binding = 1, std430) readonly buffer InstanceBuffer 
{
    Instance data[];
} Instances;

layout (set = 0, binding = 2, std430) readonly buffer LightBuffer 
{
    Light data[];
} Lights;

layout (set = 0, binding = 3) uniform accelerationStructureEXT uTopLevelAS;

layout (set = 0, binding = 4) uniform samplerCube uSkybox;
/////////////////////////////////////////////////////////////////

layout (set = 1, binding = 0, std430) readonly buffer VertexBuffer 
{
    Vertex data[];
} Vertices[];

layout (set = 2, binding = 0) readonly buffer IndexBuffer 
{
    uint data[];
} Indices[];
///////////////////////////////////////////////////////////////

layout (set = 3, binding = 0) readonly buffer SubmeshInfoBuffer 
{
    uvec2 data[];
} SubmeshInfo[];

///////////////////////////////////////////////////////////////
layout (set = 4, binding = 0) uniform sampler2D uSamplers[];


#endif