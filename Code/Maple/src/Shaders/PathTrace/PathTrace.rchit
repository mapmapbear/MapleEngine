#version 460
#extension GL_ARB_shading_language_420pack : enable
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_scalar_block_layout : enable

#include "../Common/Light.h"

#include "BRDF.glsl"

const float PBR_WORKFLOW_SEPARATE_TEXTURES = 0.0f;
const float PBR_WORKFLOW_METALLIC_ROUGHNESS = 1.0f;
const float PBR_WORKFLOW_SPECULAR_GLOSINESS = 2.0f;


////////////////////////Scene Infos////////////////////////////////
layout (set = 0, binding = 0, std430) readonly buffer MaterialBuffer 
{
    Material data[];
} Materials;

layout (set = 0, binding = 1, std430) readonly buffer TransformBuffer 
{
    Transform data[];
} Transforms;

layout (set = 0, binding = 2, std430) readonly buffer LightBuffer 
{
    Light data[];
} Lights;

layout (set = 0, binding = 3) uniform accelerationStructureEXT uTopLevelAS;

layout (set = 0, binding = 4) uniform samplerCube uSkybox;
/////////////////////////////////////////////////////////////////

layout (set = 1, binding = 0, scalar) readonly buffer VertexBuffer 
{
    Vertex data[];
} Vertices[];

layout (set = 2, binding = 0) readonly buffer IndexBuffer 
{
    uint data[];
} Indices[];
///////////////////////////////////////////////////////////////

layout (set = 3, binding = 0) readonly buffer SubmeshInfoBuffer 
{
    uvec2 data[];
} SubmeshInfo[];

///////////////////////////////////////////////////////////////
layout (set = 4, binding = 0) uniform sampler2D uSamplers[];

///////////////////////////////////////////////////////////////

layout (set = 5, binding = 0, rgba32f) readonly uniform image2D uPreviousColor;
layout (set = 6, binding = 0, rgba32f) writeonly uniform image2D uCurrentColor;


layout(push_constant) uniform PushConsts
{
    mat4 invViewProj;
    vec4 cameraPos;
    vec4 upDirection;
    vec4 rightDirection;
    uint numFrames;
    uint maxBounces;
    uint numLights;
    float accumulation;
    float shadowRayBias;
    float padding0;
    float padding1;
    float padding2;
} pushConsts;

rayPayloadInEXT PathTracePayload inPayload;

hitAttributeEXT vec2 hitAttribs;

layout(location = 1) rayPayloadEXT PathTracePayload indirectPayload;
layout(location = 2) rayPayloadEXT float visibility;


HitInfo getHitInfo()
{
    uvec2 primitiveOffsetMatIdx = SubmeshInfo[nonuniformEXT(gl_InstanceCustomIndexEXT)].data[gl_GeometryIndexEXT];
    HitInfo hitInfo;
    hitInfo.materialIndex   = primitiveOffsetMatIdx.y;
    hitInfo.primitiveOffset = primitiveOffsetMatIdx.x;
    hitInfo.primitiveId     = gl_PrimitiveID;
    return hitInfo;
}

Vertex getVertex(uint meshIdx, uint vertexIdx)
{
    return Vertices[nonuniformEXT(meshIdx)].data[vertexIdx];
}

Triangle getTriangle(in Transform transform, in HitInfo hitInfo)
{
    Triangle tri;

    uint primitiveId =  hitInfo.primitiveId + hitInfo.primitiveOffset;

    uvec3 idx = uvec3(Indices[nonuniformEXT(transform.meshIdx)].data[3 * primitiveId], 
                      Indices[nonuniformEXT(transform.meshIdx)].data[3 * primitiveId + 1],
                      Indices[nonuniformEXT(transform.meshIdx)].data[3 * primitiveId + 2]);

    tri.v0 = getVertex(transform.meshIdx, idx.x);
    tri.v1 = getVertex(transform.meshIdx, idx.y);
    tri.v2 = getVertex(transform.meshIdx, idx.z);
    return tri;
}

void transformVertex(in Transform transform, inout Vertex v)
{
    mat4 modelMat = transform.modelMatrix;
    mat3 normalMat = mat3(transform.normalMatrix);

    vec4 newPos     = modelMat * vec4(v.position,1);
    v.position      = newPos.xyz;
    v.normal.xyz    = normalMat * v.normal.xyz;
    v.tangent.xyz   = normalMat * v.tangent.xyz;
}

vec3 sampleLight(in SurfaceMaterial p, in Light light, out vec3 Wi, out float pdf)
{
    uint rayFlags = gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT;

    // Only use any-hit shaders at the first hit.
    if (inPayload.depth == 0)
        rayFlags = 0;
    
    uint  cullMask = 0xFF;
    float tmin      = 0.0001;
    float tmax      = 10000.0;
    vec3 origin = p.vertex.position.xyz + p.normal * pushConsts.shadowRayBias;

    vec3 Li = vec3(0.0f);

    if (light.type == LIGHT_DIRECTIONAL)
    {
        vec3 lightDir = -light.direction.xyz;
        Wi = lightDir;
        Li = light.color.xyz * (pow(light.intensity,1.4) + 0.1);
        pdf = 0.0f;
    } 
    else if (light.type == LIGHT_ENV)
    {
        vec2 randValue = nextVec2(inPayload.random);
        Wi = sampleCosineLobe(p.normal, randValue);
        Li = texture(uSkybox, Wi).rgb;
        pdf = pdfCosineLobe(dot(p.normal, Wi)); 
    }
    else if(light.type == LIGHT_POINT)
    {
        // Vector to light
        vec3 lightDir = light.position.xyz - p.vertex.position.xyz;
        // Distance from light to fragment position
        float dist = length(lightDir);
        // Light to fragment
        lightDir = normalize(lightDir);
        // Attenuation
        float atten = light.radius / (pow(dist, 2.0) + 1.0);
        Wi = lightDir;
        Li = light.color.xyz * (pow(light.intensity,1.4) + 0.1);
        pdf = 0.0f;
        tmax = dist;
    }
    else if(light.type == LIGHT_SPOT)
    {
        vec3 L = light.position.xyz -  p.vertex.position.xyz;
        float cutoffAngle   = 1.0f - light.angle;      
        float dist          = length(L);
        L = normalize(L);
        float theta         = dot(L.xyz, light.direction.xyz);
        float epsilon       = cutoffAngle - cutoffAngle * 0.9f;
        float attenuation 	= ((theta - cutoffAngle) / epsilon); // atteunate when approaching the outer cone
        attenuation         *= light.radius / (pow(dist, 2.0) + 1.0);//saturate(1.0f - dist / light.range);
    
        float value = clamp(attenuation, 0.0, 1.0);

        Li = light.color.xyz * (pow(light.intensity,1.4) + 0.1) * value;
        Wi = L;
        pdf = 0.0f;
        tmax = dist;
    }
    // Trace Ray
    traceRayEXT(uTopLevelAS, 
                rayFlags, 
                cullMask, 
                VISIBILITY_CLOSEST_HIT_SHADER_IDX, 
                0, 
                VISIBILITY_MISS_SHADER_IDX, 
                origin, 
                tmin, 
                Wi, 
                tmax, 
                2);
    //visibility = 1.0;
    return Li * visibility;
}


vec3 directLighting(in SurfaceMaterial p)
{
    vec3 L = vec3(0.0f);

    uint lightIdx = nextUInt(inPayload.random, pushConsts.numLights);
    const Light light = Lights.data[lightIdx];

    vec3 view = -gl_WorldRayDirectionEXT;
    vec3 lightDir = vec3(0.0f);
    vec3 halfV = vec3(0.0f);
    float pdf = 0.0f;

    vec3 Li = sampleLight(p, light, lightDir, pdf);

    halfV = normalize(lightDir + view);

    vec3 brdf = BRDF(p, view, halfV, lightDir);
    float cosTheta = clamp(dot(p.normal, lightDir), 0.0, 1.0);

    if (!isBlack(Li))
    {
        if (pdf == 0.0f)
            L = inPayload.T * brdf * cosTheta * Li;
        else
            L = (inPayload.T * brdf * cosTheta * Li) / pdf;
    }
    return L * float( pushConsts.numLights );
}

void getAlbedo(in Material material, inout SurfaceMaterial p)
{
    if (material.textureIndices0.x >= 0)
        p.albedo =  (1.0 - material.usingValue0.x) * material.albedo + material.usingValue0.x *  textureLod(uSamplers[nonuniformEXT(material.textureIndices0.x)], p.vertex.texCoord.xy, 0.0);
    else
        p.albedo =  material.albedo;
    p.albedo = gammaCorrectTexture(p.albedo);
}

vec3 getNormalFromMap(in Vertex vertex, uint normalMapIndex)
{
	vec3 tangentNormal = texture(uSamplers[nonuniformEXT(normalMapIndex)] , vertex.texCoord).xyz * 2.0 - 1.0;
	
	vec3 N = normalize(vertex.normal);
	vec3 T = normalize(vertex.tangent);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);
	
	return normalize(TBN * tangentNormal);
}

void getNormal(in Material material, inout SurfaceMaterial p)
{
    if (material.textureIndices0.y == -1)
        p.normal = p.vertex.normal.xyz;
    else
        p.normal = getNormalFromMap(p.vertex, material.textureIndices0.y);
}

void getRoughness(in Material material, inout SurfaceMaterial p)
{
    if (material.textureIndices0.z == -1 && material.usingValue1.w == PBR_WORKFLOW_SEPARATE_TEXTURES)
    {
        p.roughness = material.roughness.r;
    }
    else
    {
        if(material.usingValue1.w == PBR_WORKFLOW_SEPARATE_TEXTURES)
        {
            p.roughness = (1 - material.usingValue0.z ) * material.roughness.r +  ( material.usingValue0.z ) * textureLod(uSamplers[nonuniformEXT(material.textureIndices0.z)], p.vertex.texCoord.xy, 0.0).r;
        }
        else if(material.usingValue1.w == PBR_WORKFLOW_METALLIC_ROUGHNESS)
        {
            p.roughness = (1 - material.usingValue0.z ) * material.roughness.r +  ( material.usingValue0.z ) * textureLod(uSamplers[nonuniformEXT(material.textureIndices0.w)], p.vertex.texCoord.xy, 0.0).g;
        }
        else if(material.usingValue1.w == PBR_WORKFLOW_SPECULAR_GLOSINESS)
        {
            p.roughness = (1 - material.usingValue0.z ) * material.roughness.r + ( material.usingValue0.z ) * textureLod(uSamplers[nonuniformEXT(material.textureIndices0.w)], p.vertex.texCoord.xy, 0.0).g;
        }
    }
}

void getMetallic(in Material material, inout SurfaceMaterial p)
{
    if (material.textureIndices0.w == -1)
    {
        p.metallic = material.metallic.r;
    }
    else
    {
        if(material.usingValue1.w == PBR_WORKFLOW_SEPARATE_TEXTURES)
        {
            p.metallic = ( 1 - material.usingValue0.y ) * material.metallic.r + ( material.usingValue0.y ) *textureLod(uSamplers[nonuniformEXT(material.textureIndices0.w)], p.vertex.texCoord.xy, 0.0).r;
        }
        else if(material.usingValue1.w == PBR_WORKFLOW_METALLIC_ROUGHNESS)
        {
            p.metallic = ( 1 - material.usingValue0.y ) * material.metallic.r + ( material.usingValue0.y ) *textureLod(uSamplers[nonuniformEXT(material.textureIndices0.w)], p.vertex.texCoord.xy, 0.0).b;
        }
        else if(material.usingValue1.w == PBR_WORKFLOW_SPECULAR_GLOSINESS)
        {
            p.metallic = ( 1 - material.usingValue0.y ) * material.metallic.r + ( material.usingValue0.y ) *textureLod(uSamplers[nonuniformEXT(material.textureIndices0.w)], p.vertex.texCoord.xy, 0.0).b;
        }
    }
}


void getEmissive(in Material material, inout SurfaceMaterial p)
{
    if (material.textureIndices1.x == -1)
        p.emissive = material.emissive.rgb;
    else
        p.emissive =  ( 1 - material.usingValue1.z ) * material.emissive.rgb +  material.usingValue1.z * textureLod(uSamplers[nonuniformEXT(material.textureIndices1.x)], p.vertex.texCoord.xy, 0.0).rgb;
}


vec3 indirectLighting(in SurfaceMaterial p)
{
    vec3 Wo = -gl_WorldRayDirectionEXT;
    vec3 Wi;
    float pdf;

    vec3 brdf = sampleBRDF(p, Wo, inPayload.random, Wi, pdf);

    float cosTheta = clamp(dot(p.normal, Wi), 0.0, 1.0);

    indirectPayload.L = vec3(0.0f);
    indirectPayload.T = inPayload.T *  (brdf * cosTheta) / pdf;

    // Russian roulette
    float probability = max(indirectPayload.T.r, max(indirectPayload.T.g, indirectPayload.T.b));
    if (nextFloat(inPayload.random) > probability)
        return vec3(0.0f);
 
    // Add the energy we 'lose' by randomly terminating paths
    indirectPayload.T *= 1.0f / probability;

    indirectPayload.depth = inPayload.depth + 1;
    indirectPayload.random = inPayload.random;

    uint  rayFlags = gl_RayFlagsOpaqueEXT;
    uint  cullMask = 0xFF;
    float tmin      = 0.0001;
    float tmax      = 10000.0;  
    vec3 origin = p.vertex.position.xyz;
    // Trace Ray
    traceRayEXT(uTopLevelAS, 
            rayFlags, 
            cullMask, 
            PATH_TRACE_CLOSEST_HIT_SHADER_IDX, 
            0, 
            PATH_TRACE_MISS_SHADER_IDX, 
            origin, 
            tmin, 
            Wi, 
            tmax, 
            1);

    return indirectPayload.L;
}


void main()
{
    SurfaceMaterial surface;

    const Transform instance = Transforms.data[gl_InstanceCustomIndexEXT];
    const HitInfo hitInfo = getHitInfo();
    const Triangle triangle = getTriangle(instance, hitInfo);
    const Material material = Materials.data[hitInfo.materialIndex];

    const vec3 barycentrics = vec3(1.0 - hitAttribs.x - hitAttribs.y, hitAttribs.x, hitAttribs.y);

    surface.vertex = interpolatedVertex(triangle, barycentrics);

    transformVertex(instance, surface.vertex);

    getAlbedo(material, surface);
    getNormal(material, surface);
    getRoughness(material, surface);
    getMetallic(material, surface);
    getEmissive(material, surface);

    surface.roughness = max(surface.roughness, 0.00001);
    surface.F0 = mix(vec3(0.03), surface.albedo.xyz, surface.metallic);

    inPayload.L = vec3(0.0f);

    if (inPayload.depth == 0 && !isBlack(surface.emissive.rgb))
        inPayload.L += surface.emissive.rgb;
    
    inPayload.L += directLighting(surface);

    if ((inPayload.depth + 1) < pushConsts.maxBounces)
       inPayload.L += indirectLighting(surface);
}
