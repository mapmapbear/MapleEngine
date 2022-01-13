#version 450

layout (set = 0,binding = 0) uniform sampler2D uUVSampler;
layout (set = 0,binding = 1) uniform sampler2D uColorSampler;


layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

void main()
{
  int   size       = 6;
  float separation = 2.0;
  vec2  texSize  = textureSize(uUVSampler, 0).xy;
  vec4  uv = texture(uUVSampler, inUV);

  // Removes holes in the UV map.
  if (uv.b <= 0.0)
  {
    uv    = vec4(0.0);
    float count = 0.0;

    for (int i = -size; i <= size; ++i)
    {
      for (int j = -size; j <= size; ++j)
      {
        vec2 vUV = inUV + vec2(i, j) * separation / texSize;
        uv += texture(uUVSampler , vUV );
        count += 1.0;
      }
    }

    uv.xyz /= count;
  }

  if (uv.b <= 0.0) { outColor = vec4(0.0); return;}

  vec4  color = texture(uColorSampler, uv.xy);
  float alpha = clamp(uv.b, 0.0, 1.0);

  outColor = vec4(mix(vec3(0.0), color.rgb, alpha), alpha);
}
