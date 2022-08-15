#ifndef REPROJECTION_GLSL
#define REPROJECTION_GLSL

#define NORMAL_DISTANCE 0.1f
#define PLANE_DISTANCE 5.0f


//out of frame
//check whether reprojected pixel is inside of the screen
bool outOfFrameDisocclusionCheck(ivec2 coord, ivec2 textSize)
{
   return any(lessThan(coord, ivec2(0, 0))) || any(greaterThan(coord, textSize - ivec2(1, 1)));
}

bool normalsDisocclusionCheck(vec3 normal, vec3 prevNormal)
{
    return pow(abs(dot(normal, prevNormal)), 2) <= NORMAL_DISTANCE;
}

bool planeDistanceDisocclusionCheck(vec3 pos, vec3 prevPos, vec3 normal)
{
    vec3  toVec    = pos - prevPos;
    float distance = abs(dot(toVec, normal));// distance to plane
    return distance > PLANE_DISTANCE;
}

bool isValid(ivec2 coord, vec3 currentPos, vec3 prevPos, vec3 normal, vec3 prevNormal, ivec2 textSize)
{
    if (outOfFrameDisocclusionCheck(coord, textSize)) return false;

    if (planeDistanceDisocclusionCheck(currentPos, prevPos, normal)) return false;

    if (normalsDisocclusionCheck(normal, prevNormal)) return false;

    return true;
}

vec3 worldPositionFromDepth(vec2 texCoords, float ndcDepth, mat4 viewProjInv)
{
    // to [-1.0, 1.0] range.
    vec2 screenPos = texCoords * 2.0 - 1.0;
    vec4 ndcPos = vec4(screenPos, ndcDepth, 1.0);
    vec4 worldPos = viewProjInv * ndcPos;

    worldPos = worldPos / worldPos.w;
    return worldPos.xyz;
}

bool reproject(in ivec2 fragCoord, 
               in float depth, 
               in mat4 viewProjInv,
               in sampler2D uMotionVector,
               in sampler2D uNormalSampler,
               in sampler2D uPrevMotionVector,
               in sampler2D uPrevNormalSampler,
               in sampler2D uPrevDepthSampler,
               in sampler2D uHistoryOutput,
               in sampler2D uHistoryMoments,
               out float historyColor, 
               out vec2  historyMoments, 
               out float historyLength)
{
    const vec2 textSize     = vec2(textureSize(uHistoryOutput, 0));
    const vec2 pixelCenter  = vec2(fragCoord) + vec2(0.5f);
    const vec2 texCoord     = pixelCenter / textSize;
    const vec4 motion       = texelFetch(uMotionVector, fragCoord, 0);
    const vec4 normal       = texelFetch(uNormalSampler, fragCoord, 0);
    const vec3 worldPos     = worldPositionFromDepth(texCoord, depth, viewProjInv);

    const ivec2 historyCoord      = ivec2(pixelCenter + motion.xy * textSize);
    const vec2  historyCoordFloor = floor(fragCoord.xy) + motion.xy * textSize;
    const vec2  historyTexCoord   = texCoord + motion.xy;

    historyColor = 0.0f;
    historyMoments = vec2(0.0f);

    bool v[4];
    const ivec2 offset[4] = { 
        ivec2(0, 0), 
        ivec2(1, 0),
        ivec2(0, 1),
        ivec2(1, 1) 
    };

    bool valid = false;

    /*
     * 口口
     * 口口
     */
    for (int idx = 0; idx < 4; idx++)// one of them is valid
    {
        ivec2 loc = ivec2(historyCoordFloor) + offset[idx];

        vec4 prevMotion = texelFetch(uPrevMotionVector, loc, 0);
        vec4 prevNormal = texelFetch(uPrevNormalSampler, loc, 0);
        float prevDepth = texelFetch(uPrevDepthSampler, loc, 0).r;
        vec3  prevPos   = worldPositionFromDepth(historyTexCoord, prevDepth, viewProjInv);

        v[idx] = isValid(historyCoord, worldPos, prevPos, normal.xyz, prevNormal.xyz, ivec2(textSize));

        valid = valid || v[idx];
    }

    if (valid)
    {
        float sumw = 0;
        float x    = fract(historyCoordFloor.x);
        float y    = fract(historyCoordFloor.y);

        // bilinear weights
        float w[4] = { (1 - x) * (1 - y),
                       x * (1 - y),
                       (1 - x) * y,
                       x * y };

        historyColor = 0.0f;
        historyMoments = vec2(0.0f);

        // perform the actual bilinear interpolation
        for (int idx = 0; idx < 4; idx++)
        {
            ivec2 loc = ivec2(historyCoordFloor) + offset[idx];

            if (v[idx])
            {
                historyColor    += w[idx] * texelFetch(uHistoryOutput, loc, 0).r;
                historyMoments  += w[idx] * texelFetch(uHistoryMoments, loc, 0).rg;
                sumw += w[idx];
            }
        }

        valid           = (sumw >= 0.01);
        historyColor   = valid ? historyColor / sumw : 0.0f;
        historyMoments = valid ? historyMoments / sumw : vec2(0.0f);
    }
    
    if (!valid) 
    {
        float count = 0.0;

        // performs a binary descision
        const int radius = 1;
        for (int yy = -radius; yy <= radius; yy++)
        {
            for (int xx = -radius; xx <= radius; xx++)
            {
                ivec2 p = historyCoord + ivec2(xx, yy);
                vec4  prevNormal = texelFetch(uPrevNormalSampler, p, 0);
                float prevDepth  = texelFetch(uPrevDepthSampler, p, 0).r;
                vec3  prevPos = worldPositionFromDepth(historyTexCoord, prevDepth, viewProjInv);

                if (isValid(historyCoord, worldPos, prevPos, normal.xyz, prevNormal.xyz, ivec2(textSize)))
                {
                    historyColor    += texelFetch(uHistoryOutput, p, 0).r;
                    historyMoments  += texelFetch(uHistoryMoments, p, 0).rg;
                    count += 1.0;
                }
            }
        }

        if (count > 0)
        {
            valid = true;
            historyColor /= count;
            historyMoments /= count;
        }
    }

    if (valid)
    {
        historyLength = texelFetch(uHistoryMoments, historyCoord, 0).b;
    }    
    else
    {
        historyColor = 0.0f;
        historyMoments = vec2(0.0f);
        historyLength  = 0.0f;
    }
    return valid;
}

#endif