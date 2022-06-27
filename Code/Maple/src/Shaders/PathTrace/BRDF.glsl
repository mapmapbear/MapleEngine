#ifndef BRDF_GLSL
#define BRDF_GLSL

#include "Common.glsl"

const float EPSILON = 0.00001;
// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2
float ndfGGX(float cosLh, float roughness)
{
	float alpha = roughness * roughness;
	float alphaSq = alpha * alpha;
	
	float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (M_PI * denom * denom);
}

// Single term for separable Schlick-GGX below.
float gaSchlickG1(float cosTheta, float k)
{
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method.
float gaSchlickGGX(float cosLi, float NdotV, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights.
	return gaSchlickG1(cosLi, k) * gaSchlickG1(NdotV, k);
}

// Shlick's approximation of the Fresnel factor.
vec3 fresnelSchlick(vec3 F0, float cosTheta)
{
  	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 BRDF(in SurfaceMaterial p, in vec3 Wo, in vec3 Wh, in vec3 Wi)
{
    float NdotL = max(dot(p.normal, Wi), 0.0);
    float NdotV = max(dot(p.normal, Wo), 0.0);
    float NdotH = max(dot(p.normal, Wh), 0.0);
    float VdotH = max(dot(Wi, Wh), 0.0);

    vec3 F  = fresnelSchlick(p.F0, VdotH);
    float D = ndfGGX(NdotH,p.roughness);
    float G = gaSchlickGGX(NdotL,NdotV,p.roughness);

    vec3 kd = (1.0 - F) * (1.0 - p.metallic);
	vec3 diffuseBRDF = kd * p.albedo.xyz / M_PI;
    vec3 specularBRDF = (F * D * G) / max(EPSILON, 4.0 * NdotL * NdotV);
    return diffuseBRDF + specularBRDF;
}

#endif