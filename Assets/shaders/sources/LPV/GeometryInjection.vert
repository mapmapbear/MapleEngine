#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#include "LPVCommon.glsl"

layout(set = 0, binding = 0) uniform UniformBufferObject
{
    int textureSize;
    int rsmSize;
    vec3 lightDirection;
}ubo;


layout(set = 0, binding = 1) uniform sampler2D uFluxSampler;
layout(set = 0, binding = 2) uniform sampler2D uRsmWorldPosition;
layout(set = 0, binding = 3) uniform sampler2D uRsmWorldNormal;


layout( location = 0 ) out float outSurfelArea;
layout( location = 1 ) out RSMTexel outRsmTexel;

// Get ndc texture coordinates from gridcell
vec2 getRenderingTexCoords(ivec3 gridCell)
{
	float textureSize = float(ubo.textureSize);
	// Displace int coordinates with 0.5
	vec2 texCoords = vec2((gridCell.x % ubo.textureSize) + ubo.textureSize * gridCell.z, gridCell.y) + vec2(0.5);
	// Get ndc coordinates
	vec2 ndc = vec2((2.0 * texCoords.x) / (textureSize * textureSize), (2.0 * texCoords.y) / textureSize) - vec2(1.0);
	return ndc;
}

// Sample from light
float calculateSurfelAreaLight(vec3 lightPos)
{
    float fov = 90.0f; //TODO fix correct fov
    float aspect = float(ubo.rsmSize / ubo.rsmSize);
    float tanFovxHalf = tan(0.5 * fov * DEG_TO_RAD);
    float tanFovyHalf = tan(0.5 * fov * DEG_TO_RAD) * aspect;

	return (4.0 * lightPos.z * lightPos.z * tanFovxHalf * tanFovyHalf) / float(ubo.rsmSize * ubo.rsmSize);
}

void main()
{
	ivec2 rsmTexCoords = ivec2(gl_VertexIndex % ubo.rsmSize, gl_VertexIndex /  ubo.rsmSize);

	outRsmTexel.worldNormal = texelFetch(uRsmWorldNormal, rsmTexCoords, 0).xyz;
	outRsmTexel.worldPosition = texelFetch(uRsmWorldPosition, rsmTexCoords, 0).xyz + 0.5  *  outRsmTexel.worldNormal;
	outRsmTexel.flux = texelFetch(uFluxSampler, rsmTexCoords, 0);

	ivec3 gridCell = getGridCelli(outRsmTexel.worldPosition, ubo.textureSize);

	vec2 texCoord = getRenderingTexCoords(gridCell);

	gl_PointSize = 1.0;
	gl_Position = vec4(texCoord, 0.0, 1.0);

    outSurfelArea = calculateSurfelAreaLight(ubo.lightDirection);
}