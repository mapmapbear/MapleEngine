#version 450

layout (set = 0,binding = 0) uniform sampler2D uReflectionColorSampler;
layout (set = 0,binding = 1) uniform sampler2D uReflectionBlurSampler;
layout (set = 0,binding = 2) uniform sampler2D uPBRSampler;


layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;


void main()
{
  vec2 texSize  = textureSize(uReflectionColorSampler, 0).xy;

  vec4 pbr       = texture(uPBRSampler, inUV);
  vec4 color     = texture(uReflectionColorSampler, inUV);
  vec4 colorBlur = texture(uReflectionBlurSampler, inUV);

  float metallic = clamp(pbr.r, 0.0, 1.0); //metallic

  if (metallic <= 0.0) { outColor = vec4(0.0); return; }

  float roughness = clamp(pbr.g, 0.0, 1.0);

  outColor = mix(color, colorBlur, roughness) * metallic;
}