#ifndef DESCRIPTOR_SET_3
#define DESCRIPTOR_SET_3

layout(set = 3, binding = 0, std430) buffer DenoiseTileData
{
    ivec2 coord[];
}denoiseTileData;

layout(set = 3, binding = 1, std430) buffer DenoiseTileDispatchArgs
{
    uint numGroupsX;
    uint numGroupsY;
    uint numGroupsZ;
}denoiseTileDispatchArgs;

layout(set = 3, binding = 2, std430) buffer ShadowTileData
{
    ivec2 coord[];
}shadowTileData;

layout(set = 3, binding = 3, std430) buffer ShadowTileDispatchArgs
{
    uint numGroupsX;
    uint numGroupsY;
    uint numGroupsZ;
}shadowTileDispatchArgs;

#endif