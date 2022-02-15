#version 450

#include "LPVCommon.glsl"




layout(location = 0) out vec4 outRedColor;
layout(location = 1) out vec4 outGreenColor;
layout(location = 2) out vec4 outBlueColor;


layout(set = 1, binding = 0) uniform UniformBufferObject
{
    int gridSize;
    int rsmSize;
}ubo;

layout(location = 0) in RSMTexel inRsmTexel;

void main()
{
	float surfelWeight = float(ubo.gridSize) / float(ubo.rsmSize);
	vec4 SHCoeffs = (evalCosineLobeToDir(inRsmTexel.worldNormal) / PI) * surfelWeight;

	vec4 shR = SHCoeffs * inRsmTexel.flux.r;
	vec4 shG = SHCoeffs * inRsmTexel.flux.g;
	vec4 shB = SHCoeffs * inRsmTexel.flux.b;

	outRedColor = shR;
	outGreenColor = shG;
	outBlueColor = shB;
}