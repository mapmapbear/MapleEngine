#version 450

layout(set = 0, binding = 0) uniform sampler2D uScreenSampler;

layout (location = 0) out vec4 outColor;

layout (location = 0) in vec2 inUV;

void main() 
{
    outColor = texture(uScreenSampler,inUV);
}