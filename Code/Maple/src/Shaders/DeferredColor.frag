#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : require

#include "Common/Material.h"

#define PI 3.1415926535897932384626433832795
#define GAMMA 2.2
const float PBR_WORKFLOW_SEPARATE_TEXTURES = 0.0f;
const float PBR_WORKFLOW_METALLIC_ROUGHNESS = 1.0f;
const float PBR_WORKFLOW_SPECULAR_GLOSINESS = 2.0f;

layout(location = 0) in vec4 fragPosition;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in vec3 fragNormal;
layout(location = 4) in vec3 fragTangent;
layout(location = 5) in vec4 fragProjPosition;
layout(location = 6) in vec4 fragOldProjPosition;
layout(location = 7) in vec4 fragViewPosition;

layout(set = 1, binding = 0) uniform sampler2D uAlbedoMap;
layout(set = 1, binding = 1) uniform sampler2D uMetallicMap;
layout(set = 1, binding = 2) uniform sampler2D uRoughnessMap;
layout(set = 1, binding = 3) uniform sampler2D uNormalMap;
layout(set = 1, binding = 4) uniform sampler2D uAOMap;
layout(set = 1, binding = 5) uniform sampler2D uEmissiveMap;
layout(set = 1, binding = 6) uniform UniformMaterialData
{
	Material material;
} materialProperties;


layout(set = 2,binding = 0) uniform UBO
{
	mat4 view;
	float nearPlane;
	float farPlane;
	float padding;
	float padding2;
}ubo;

//bind to framebuffer
layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outPosition;
layout(location = 2) out vec4 outNormal;
layout(location = 3) out vec4 outPBR;

layout(location = 4) out vec4 outViewPosition;
layout(location = 5) out vec4 outViewNormal;
layout(location = 6) out vec4 outVelocity;

vec4 gammaCorrectTexture(vec4 samp)
{
	return vec4(pow(samp.rgb, vec3(GAMMA)), samp.a);
}

vec3 gammaCorrectTextureRGB(vec4 samp)
{
	return vec3(pow(samp.rgb, vec3(GAMMA)));
}

vec4 getAlbedo()
{
	return (1.0 - materialProperties.material.usingAlbedoMap) * materialProperties.material.albedoColor + materialProperties.material.usingAlbedoMap * texture(uAlbedoMap, fragTexCoord);
}

vec3 getMetallic()
{
	return (1.0 - materialProperties.material.usingMetallicMap) * materialProperties.material.metallicColor.rgb + materialProperties.material.usingMetallicMap * texture(uMetallicMap, fragTexCoord).rgb;
}

float getRoughness()
{
	return (1.0 - materialProperties.material.usingRoughnessMap) *  materialProperties.material.roughnessColor.r + materialProperties.material.usingRoughnessMap * texture(uRoughnessMap, fragTexCoord).r;
}

float getAO()
{
	return (1.0 - materialProperties.material.usingAOMap) + materialProperties.material.usingAOMap * gammaCorrectTextureRGB(texture(uAOMap, fragTexCoord)).r;
}

vec3 getEmissive()
{
	return (1.0 - materialProperties.material.usingEmissiveMap) * materialProperties.material.emissiveColor.rgb + materialProperties.material.usingEmissiveMap * gammaCorrectTextureRGB(texture(uEmissiveMap, fragTexCoord));
}

vec3 getNormalFromMap()
{
	if (materialProperties.material.usingNormalMap < 0.1)
		return normalize(fragNormal);
	
	vec3 tangentNormal = texture(uNormalMap, fragTexCoord).xyz * 2.0 - 1.0;
	
	/*vec3 Q1 = dFdx(fragPosition.xyz);
	vec3 Q2 = dFdy(fragPosition.xyz);
	vec2 st1 = dFdx(fragTexCoord);
	vec2 st2 = dFdy(fragTexCoord);*/
	
	vec3 N = normalize(fragNormal);
	//vec3 T = normalize(Q1*st2.t - Q2*st1.t);
	vec3 T = fragTangent;
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);
	
	return normalize(TBN * tangentNormal);
}


float linearDepth(float depth)
{
	float z = depth * 2.0f - 1.0f; 
	return (2.0f * ubo.nearPlane * ubo.farPlane) / (ubo.farPlane + ubo.nearPlane - z * (ubo.farPlane - ubo.nearPlane));	
}


void main()
{
	vec4 texColor = getAlbedo() * fragColor;
	if(texColor.w < 0.01)
		discard;

	float metallic = 0.0;
	float roughness = 0.0;
	float ao		= getAO();

	if(materialProperties.material.workflow == PBR_WORKFLOW_SEPARATE_TEXTURES)
	{
		metallic  = getMetallic().x;
		roughness = getRoughness();
	}
	else if( materialProperties.material.workflow == PBR_WORKFLOW_METALLIC_ROUGHNESS)
	{
		vec3 tex = texture(uMetallicMap, fragTexCoord).rgb;
		//ao  	  = tex.r;
		metallic  = tex.b;
 		roughness = tex.g;
	}
	else if( materialProperties.material.workflow == PBR_WORKFLOW_SPECULAR_GLOSINESS)
	{
		vec3 tex = texture(uMetallicMap, fragTexCoord).rgb;
		metallic = tex.b;
		roughness = tex.g * materialProperties.material.roughnessColor.r;
	}

	vec3 emissive   = getEmissive();
 
    outColor    	= gammaCorrectTexture(texColor);
	outPosition		= vec4(fragPosition.xyz, emissive.x);
	outNormal   	= vec4(getNormalFromMap(), emissive.y);
	outPBR      	= vec4(metallic, roughness, ao, emissive.z);

	outViewPosition = fragViewPosition;
	outViewNormal   = vec4(transpose(inverse(mat3(ubo.view))) * outNormal.xyz, 1);
	//outViewNormal   = ubo.view * outNormal;
    vec2 a = (fragProjPosition.xy / fragProjPosition.w) * 0.5 + 0.5;
    vec2 b = (fragOldProjPosition.xy / fragOldProjPosition.w) * 0.5 + 0.5;
    outVelocity.xy = a - b;
}