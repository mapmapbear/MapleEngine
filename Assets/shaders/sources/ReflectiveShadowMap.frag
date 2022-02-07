#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout( location = 0 ) in vec4 inPosition;
layout( location = 1 ) in vec2 inUV;
layout( location = 2 ) in vec3 inNormal;

layout(set = 1, binding = 0) uniform sampler2D uDiffuseMap;

layout(set = 1 , binding = 1) uniform UBO
{
    vec4 color;
	float type;
}ubo;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outPosition;
layout(location = 2) out vec4 outNormal;

void main()
{
	vec3 diffuse = texture(uDiffuseMap, inUV).rgb;
	vec4 flux = vec4(0.0);

	if( ubo.type == 0.0 )
    {
		flux = vec4( ( ubo.color.rgb * diffuse ) , 1.0 );
    }

	outColor = flux;
	outPosition = inPosition;
	outNormal = vec4(inNormal,1.0);
}