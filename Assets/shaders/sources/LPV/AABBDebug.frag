#version 450

#include "LPVCommon.glsl"

layout (location = 0) in vec3 inPosition; // in view space
layout (location = 1) in vec3 inNormal;
layout (location = 2) in flat ivec3 inVolumeCellIndex;

layout(set = 1, binding = 0) uniform UniformBufferObjectFrag
{
	vec3 minAABB;
	float cellSize;
}ubo;

layout(set = 1, binding = 1) uniform sampler3D uRAccumulatorLPV;
layout(set = 1, binding = 2) uniform sampler3D uGAccumulatorLPV;
layout(set = 1, binding = 3) uniform sampler3D uBAccumulatorLPV;

layout (location = 0) out vec4 outColor;

ivec3 convertPointToGridIndex(vec3 vPos) {
	return ivec3((vPos - ubo.minAABB) / ubo.cellSize);
}

void main()
{
	vec4 shIntensity = dirToSH(normalize(inNormal));
	ivec3 cellCoords = convertPointToGridIndex(inPosition);
	vec3 lpvIntensity = vec3( 
			dot(shIntensity, texelFetch(uRAccumulatorLPV, cellCoords,0) ),
			dot(shIntensity, texelFetch(uGAccumulatorLPV, cellCoords,0) ),
			dot(shIntensity, texelFetch(uBAccumulatorLPV, cellCoords,0) )
		);
	outColor = vec4(max(shIntensity.rgb, 0 ) * 10,1);
}
