#version 450


layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragPosition;
layout(location = 3) in vec3 fragNormal;
layout(location = 4) in vec3 fragTangent;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outPosition;
layout(location = 2) out vec4 outNormal;
layout(location = 3) out vec4 outPBR;

void main() 
{
 	outColor    = vec4(vec3(0.0), 1.0); 
	outPosition	= vec4(fragPosition.xyz,1);
	outNormal   = vec4(fragNormal,1);
	outPBR      = vec4(0,0,0,1);
}