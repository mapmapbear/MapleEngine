#ifndef PREV_GBUFFER
#define PREV_GBUFFER

layout(set = 2, binding = 0) uniform sampler2D uPrevPositionSampler;
layout(set = 2, binding = 1) uniform sampler2D uPrevNormalSampler;
layout(set = 2, binding = 2) uniform sampler2D uPrevDepthSampler;
layout(set = 2, binding = 3) uniform sampler2D uPrevVelocitySampler;

#endif
