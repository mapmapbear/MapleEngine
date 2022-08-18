#ifndef GBUFFER_
#define GBUFFER_

layout(set = 1, binding = 0) uniform sampler2D uPositionSampler;
layout(set = 1, binding = 1) uniform sampler2D uNormalSampler;
layout(set = 1, binding = 2) uniform sampler2D uDepthSampler;
layout(set = 1, binding = 3) uniform sampler2D uVelocitySampler;


#endif
