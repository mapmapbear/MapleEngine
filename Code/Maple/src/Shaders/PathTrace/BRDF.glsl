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

vec3 BRDF(in SurfaceMaterial p, in vec3 view, in vec3 halfV, in vec3 lightDir)
{
	float cosLi = max(0.0, dot(p.normal, lightDir));
    float cosLh = max(0.0, dot(p.normal, halfV));
    float NdotV = max(0.0, dot(p.normal, view));

    vec3 F  = fresnelSchlick(p.F0, max(dot(halfV, view), 0.0));
    float D = ndfGGX(cosLh,p.roughness);
    float G = gaSchlickGGX(cosLi,NdotV,p.roughness);

    vec3 kd = (1.0 - F) * (1.0 - p.metallic);
	vec3 diffuseBRDF = kd * p.albedo.xyz / M_PI;
    vec3 specularBRDF = (F * D * G) / max(EPSILON, 4.0 * cosLi * NdotV);
    return diffuseBRDF + specularBRDF;
}


vec3 sampleCosineLobe(in vec3 n, in vec2 r)
{
    vec2 randSample = max(vec2(0.00001f), r);

    const float phi = 2.0f * M_PI * randSample.y;

    const float cosTheta = sqrt(randSample.x);
    const float sinTheta = sqrt(1 - randSample.x);

    vec3 t = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

    return normalize(makeRotationMatrix(n) * t);
}

float pdfCosineLobe(in float ndotl)
{
    return ndotl / M_PI;
}
#endif