#version 450

const int SSAO_KERNEL_SIZE = 64;
const float SSAO_RADIUS = 0.5;

layout (set = 0,binding = 0) uniform sampler2D uPositionSampler;
layout (set = 0,binding = 1) uniform sampler2D uNormalSampler;
layout (set = 0,binding = 2) uniform sampler2D uSsaoNoise;
layout (set = 0,binding = 3) uniform UBOSSAOKernel
{
	vec4 samples[SSAO_KERNEL_SIZE];
} uboSSAOKernel;

layout (set = 0,binding = 4) uniform UBO 
{
	mat4 projection;
} ubo;

layout (location = 0) in vec2 inUV;

layout (location = 0) out float outColor;

void main() 
{
	// Get G-Buffer values
	vec3 fragPos = texture(uPositionSampler, inUV).rgb;
	vec3 normal = normalize(texture(uNormalSampler, inUV).rgb * 2.0 - 1.0);

	// Get a random vector using a noise lookup
	ivec2 texDim = textureSize(uPositionSampler, 0); 
	ivec2 noiseDim = textureSize(uSsaoNoise, 0);
	const vec2 noiseUV = vec2(float(texDim.x)/float(noiseDim.x), float(texDim.y)/(noiseDim.y)) * inUV;  
	vec3 randomVec = texture(uSsaoNoise, noiseUV).xyz * 2.0 - 1.0;
	
	// Create TBN matrix
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(tangent, normal);
	mat3 TBN = mat3(tangent, bitangent, normal);

	// Calculate occlusion value
	float occlusion = 0.0f;
	// remove banding
	const float bias = 0.025f;
	for(int i = 0; i < SSAO_KERNEL_SIZE; i++)
	{		
		vec3 samplePos = TBN * uboSSAOKernel.samples[i].xyz; 
		samplePos = fragPos + samplePos * SSAO_RADIUS; 
		
		// project
		vec4 offset = vec4(samplePos, 1.0f);
		offset = ubo.projection * offset; 
		offset.xyz /= offset.w; 
		offset.xyz = offset.xyz * 0.5f + 0.5f; 
		
		float sampleDepth = -texture(uPositionSampler, offset.xy).w; 

		float rangeCheck = smoothstep(0.0f, 1.0f, SSAO_RADIUS / abs(fragPos.z - sampleDepth));
		occlusion += (sampleDepth >= samplePos.z + bias ? 1.0f : 0.0f) * rangeCheck;           
	}
	occlusion = 1.0 - (occlusion / float(SSAO_KERNEL_SIZE));
	
	outColor = occlusion;
}

