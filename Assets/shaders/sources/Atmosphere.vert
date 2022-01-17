#version 450 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;


layout(set = 0,binding = 0) uniform UniformBufferObject
{
	mat4 projView;
} ubo;

layout(push_constant) uniform PushConsts
{
	mat4 transform;
} pushConsts;


layout(location = 0) out vec4 fragPosition;

void main()
{
    fragPosition = pushConsts.transform * vec4(inPosition, 1.0);
	gl_Position = ubo.projView * fragPosition;
}

