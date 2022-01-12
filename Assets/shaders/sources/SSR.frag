#version 450

layout(set = 0, binding = 0) uniform sampler2D uScreenSampler;
layout(set = 0, binding = 1) uniform sampler2D uViewPositionSampler;
layout(set = 0, binding = 2) uniform sampler2D uViewNormalSampler;
layout(set = 0, binding = 3) uniform sampler2D uPBRSampler;
layout(set = 0, binding = 4) uniform UniformBufferObject
{
    mat4 view;
    mat4 projection;
} ubo;

layout (location = 0) out vec4 outColor;

layout (location = 0) in vec2 inUV;


const float step = 0.1;
const float minRayStep = 0.1;
const float maxSteps = 30;
const int numBinarySearchSteps = 5;
const float reflectionSpecularFalloffExponent = 3.0;
const float strength = 0.3;

#define Scale vec3(.8, .8, .8)
#define K 19.19

vec3 binarySearch(inout vec3 dir, inout vec3 hitCoord, inout float dDepth);
vec4 rayMarch(vec3 dir, inout vec3 hitCoord, out float dDepth);
vec3 fresnelSchlick(float cosTheta, vec3 F0);
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness);
vec3 hash(vec3 a);


void main()
{
    vec3 albedo = texture(uScreenSampler, inUV).rgb;

    float metallic = texture(uPBRSampler, inUV).r;
    float roughness = texture(uPBRSampler, inUV).g;
    
    if(1 - roughness < 0.1){
        outColor = vec4(albedo, 1.0);
        return;
    }
 
    //vec3 viewNormal = transpose(inverse(mat3(ubo.view))) * texture(uNormalSampler, inUV).xyz;
    //vec3 viewPos =  (ubo.view * texture(uPositionSampler, inUV)).xyz;

    vec3 viewNormal = texture(uViewNormalSampler, inUV).xyz;
    vec3 viewPos = texture(uViewPositionSampler, inUV).xyz;

    vec3 F0 = vec3(0.04); 
    F0      = mix(F0, albedo, metallic);
    vec3 Fresnel = fresnelSchlickRoughness(max(dot(normalize(viewNormal), normalize(viewPos)), 0.0), F0, roughness);

    // Reflection vector
    vec3 reflected = normalize(reflect(normalize(viewPos), normalize(viewNormal)));


    vec3 hitPos = viewPos;
    float dDepth;
 
    vec3 wp = vec3(vec4(viewPos, 1.0));
    vec3 jitt = mix(vec3(0.0), vec3(hash(wp)), min(roughness, 0.01));
    vec4 coords = rayMarch((vec3(jitt) + reflected * max(minRayStep, -viewPos.z)), hitPos, dDepth);
 
 
    vec2 dCoords = smoothstep(0.2, 0.6, abs(vec2(0.5, 0.5) - coords.xy));
 
 
    float screenEdgefactor = clamp(1.0 - (dCoords.x + dCoords.y), 0.0, 1.0);

    float ReflectionMultiplier = pow(1 - roughness, reflectionSpecularFalloffExponent) * 
                screenEdgefactor * 
                -reflected.z;
 
    // Get color
    vec4 SSR = vec4(textureLod(uScreenSampler, coords.xy, 0).rgb, clamp(ReflectionMultiplier * Fresnel * strength, 0.0, 0.9));  

    vec3 blending =  SSR.rgb * SSR.a + albedo * (1.0 - SSR.a);
    outColor = vec4(blending, 1.0);
}

vec3 binarySearch(inout vec3 dir, inout vec3 hitCoord, inout float dDepth)
{
    float depth;

    vec4 projectedCoord;
 
    for(int i = 0; i < numBinarySearchSteps; i++)
    {

        projectedCoord = ubo.projection * vec4(hitCoord, 1.0);
        projectedCoord.xy /= projectedCoord.w;
        projectedCoord.xy = projectedCoord.xy * 0.5 + 0.5;
 
        depth = texture(uViewPositionSampler, projectedCoord.xy).z;
 
        dDepth = hitCoord.z - depth;

        dir *= 0.5;
        if(dDepth > 0.0)
            hitCoord += dir;
        else
            hitCoord -= dir;    
    }

        projectedCoord = ubo.projection * vec4(hitCoord, 1.0);
        projectedCoord.xy /= projectedCoord.w;
        projectedCoord.xy = projectedCoord.xy * 0.5 + 0.5;
 
    return vec3(projectedCoord.xy, depth);
}

vec4 rayMarch(vec3 dir, inout vec3 hitCoord, out float dDepth)
{
    dir *= step;
 
    float depth;
    int steps;
    vec4 projectedCoord;

 
    for(int i = 0; i < maxSteps; i++)
    {
        hitCoord += dir;
 
        projectedCoord = ubo.projection * vec4(hitCoord, 1.0);
        projectedCoord.xy /= projectedCoord.w;
        projectedCoord.xy = projectedCoord.xy * 0.5 + 0.5;
 
        depth = texture(uViewPositionSampler, projectedCoord.xy).z;
        if(depth > 1000.0)
            continue;
 
        dDepth = hitCoord.z - depth;

        if((dir.z - dDepth) < 1.2)
        {
            if(dDepth <= 0.0)
            {   
                vec4 Result;
                Result = vec4(binarySearch(dir, hitCoord, dDepth), 1.0);

                return Result;
            }
        }
        
        steps++;
    }
    return vec4(projectedCoord.xy, depth, 0.0);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness){
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}  

vec3 hash(vec3 a)
{
    a = fract(a * Scale);
    a += dot(a, a.yxz + K);
    return fract((a.xxy + a.yxx)*a.zyx);
}