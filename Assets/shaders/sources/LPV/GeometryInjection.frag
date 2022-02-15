#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#include "LPVCommon.glsl"

layout(location = 0) in float inSurfelArea;
layout(location = 1) in RSMTexel inRsmTexel;


layout(location = 0) out vec4 outRedColor;
layout(location = 1) out vec4 outGreenColor;
layout(location = 2) out vec4 outBlueColor;


layout(set = 1, binding = 0) uniform UniformBufferObject
{
    vec3 lightDirection;
}ubo;

float calculateBlockingPotencial(vec3 dir, vec3 normal)
{
	return clamp((inSurfelArea * clamp(dot(normal, dir), 0.0, 1.0)) / (CELLSIZE * CELLSIZE), 0.0, 1.0); //It is probability so 0.0 - 1.0
}

void main()
{
    //Discard pixels with really small normal
	if (length(inRsmTexel.worldNormal) < 0.01)
    {
		discard;
	}

	vec3 lightDir = normalize(ubo.lightDirection - inRsmTexel.worldPosition); //Both are in world space
	float blockingPotencial = calculateBlockingPotencial(lightDir, inRsmTexel.worldNormal);

	vec4 SH_coeffs = evalCosineLobeToDir(inRsmTexel.worldNormal) * blockingPotencial;
	vec4 shR = SH_coeffs * inRsmTexel.flux.r;
    vec4 shG = SH_coeffs * inRsmTexel.flux.g;
    vec4 shB = SH_coeffs * inRsmTexel.flux.b;

	outRedColor = shR;
    outGreenColor = shG;
    outBlueColor = shB;
}