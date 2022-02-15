#version 450

#include "LPVCommon.glsl"

layout(location = 0) in vec2 inPosition;

layout(set = 0, binding = 0) uniform UniformBufferObject
{
    int gridSize;
    int rsmSize;
}ubo;

layout(set = 0, binding = 1) uniform sampler2D uFluxSampler;
layout(set = 0, binding = 2) uniform sampler2D uRsmWorldPosition;
layout(set = 0, binding = 3) uniform sampler2D uRsmWorldNormal;

layout(location = 0) out RSMTexel outRsmTexel;

RSMTexel getRsmTexel(ivec2 texCoord) 
{
	RSMTexel texel;
	texel.worldNormal = texelFetch(uRsmWorldNormal, texCoord, 0).xyz;

	// Displace the position by half a normal
	texel.worldPosition = texelFetch(uRsmWorldPosition, texCoord, 0).xyz + 0.5 * CELLSIZE * texel.worldNormal;
	texel.flux = texelFetch(uFluxSampler, texCoord, 0);
	return texel;
}

// Get ndc texture coordinates from gridcell
vec2 getGridOutputPosition(ivec3 gridCell)
{
	float textureSize = float(ubo.gridSize);
	// Displace int coordinates with 0.5
	vec2 texCoords = vec2((gridCell.x % ubo.gridSize) + ubo.gridSize * gridCell.z, gridCell.y) + vec2(0.5);
	// Get ndc coordinates
	vec2 ndc = vec2((2.0 * texCoords.x) / (textureSize * textureSize), (2.0 * texCoords.y) / textureSize) - vec2(1.0);
	return ndc;
}

void main()
{
	ivec2 rsmTexCoords = ivec2(gl_VertexIndex % ubo.rsmSize, gl_VertexIndex /  ubo.rsmSize);
	outRsmTexel = getRsmTexel(rsmTexCoords);
	ivec3 gridCell = getGridCelli(outRsmTexel.worldPosition, ubo.gridSize);

	vec2 texCoord = getGridOutputPosition(gridCell);

	gl_PointSize = 1.0;
	gl_Position = vec4(texCoord, 0.0, 1.0);
}