#version 450

layout (set = 0,binding = 0) uniform sampler2D uReflectionColorSampler;
layout (set = 0,binding = 1) uniform UniformBufferObject
{
  vec2 parameters;
} ubo;


layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;


void main()
{
  vec2 texSize  = textureSize(uReflectionColorSampler, 0).xy;

  int size = int(ubo.parameters.x);
  if (size <= 0) { return; }

  float separation = max(ubo.parameters.y,1);
  vec4 fragColor = vec4(0);

  float count = 0.0;

  for (int i = -size; i <= size; ++i) 
  {
    for (int j = -size; j <= size; ++j)
    {
      vec2 vUV = inUV + vec2(i, j) * separation / texSize;
      fragColor += texture(uReflectionColorSampler, vUV);
      count += 1.0;
    }
  }
  outColor = fragColor / count;
}
