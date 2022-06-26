#ifndef MATERIAL_H
#define MATERIAL_H

struct Material
{
    vec4  albedoColor;
	vec4  roughnessColor;
	vec4  metallicColor;
	vec4  emissiveColor;
	float usingAlbedoMap;
	float usingMetallicMap;
	float usingRoughnessMap;
	float usingNormalMap;
	float usingAOMap;
	float usingEmissiveMap;
	float workflow;
	float padding;
};

#endif