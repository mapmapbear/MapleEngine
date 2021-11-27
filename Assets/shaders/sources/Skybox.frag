#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec4 outPosition;

layout(set = 0, binding = 1) uniform samplerCube uCubeMap;

layout(location = 0) out vec4 outFrag;

const float coeiff = 0.3;
const vec3 totalSkyLight = vec3(0.3, 0.5, 1.0);

const float PI = 3.14159265359;
const float blurLevel = 1.0f;
const float timeCounter = 0.0f;

vec3 mie(float dist, vec3 sunL)
{
	return max(exp(-pow(dist, 0.25)) * sunL - 0.4, 0.0);
}

vec3 getSky()
{
	vec3 uv = normalize(outPosition.xyz);

	vec3 sunPos;
	float radian = PI * 2.0 *((timeCounter / 86400.0) - 0.333333);
	sunPos.x = cos(radian);
	sunPos.y = sin(radian);
	sunPos.z = 0.0;

	float sunDistance = length(uv - normalize(sunPos));

	float scatterMult = clamp(sunDistance, 0.0, 1.0);

	float dist = uv.y + 0.1; 
	dist = (coeiff * mix(scatterMult, 1.0, dist)) / dist;


	vec3 color = dist * totalSkyLight;

	color = max(color, 0.0);
	color = max(mix(pow(color, 1.0 - color),
		color / (2.0 * color + 0.5 - color),
		clamp(sunPos.y * 2.0, 0.0, 1.0)), 0.0);

	return color;
}

void main()
{
	vec3 color;

	int cubeMapOnly = 0;
	if(cubeMapOnly > 0.0)
	{
		color = textureLod(uCubeMap, outPosition.xyz, blurLevel).xyz;
	}
	else
	{
		color = getSky();
	}

	outFrag = vec4(color, 1.0);
}


