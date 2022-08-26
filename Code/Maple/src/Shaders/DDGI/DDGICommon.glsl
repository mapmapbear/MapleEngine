#ifndef DDGI_COMMON
#define DDGI_COMMON

#include "../PathTrace/Random.glsl"
#include "../Common/Math.h"

struct DDGIUniform
{
    vec4  startPosition;
    vec4  step;
    ivec4 probeCounts;

    float maxDistance;
    float sharpness;
    float hysteresis;
    float normalBias;
    
    float energyPreservation;
    int   irradianceProbeSideLength;
    int   irradianceTextureWidth;
    int   irradianceTextureHeight;
    
    int   depthProbeSideLength;
    int   depthTextureWidth;
    int   depthTextureHeight;
    int   raysPerProbe;
};

struct GIPayload
{
    vec3  L;
    vec3  T;
    float hitDistance;
    Random random;
};

// Compute normalized oct coord, mapping top left of top left pixel to (-1,-1)
vec2 normalizedOctCoord(ivec2 fragCoord, int probeSideLength)
{
    int probeWithBorderSide = probeSideLength + 2;

    vec2 octFragCoord = ivec2((fragCoord.x - 2) % probeWithBorderSide, (fragCoord.y - 2) % probeWithBorderSide);
    // Add back the half pixel to get pixel center normalized coordinates
    return (vec2(octFragCoord) + vec2(0.5f)) * (2.0f / float(probeSideLength)) - vec2(1.0f, 1.0f);
}

int getProbeId(vec2 texel, int width, int probeSideLength)
{
    int probeWithBorderSide = probeSideLength + 2;
    int probesPerSide     = (width - 2) / probeWithBorderSide;
    return int(texel.x / probeWithBorderSide) + probesPerSide * int(texel.y / probeWithBorderSide);
}

vec3 gridToPosition(in DDGIUniform ddgi, ivec3 c)
{
    return ddgi.step.xyz * vec3(c) + ddgi.startPosition.xyz;
}

vec3 probeLocation(in DDGIUniform ddgi, int index)
{
    ivec3 gridCoord;
    gridCoord.x = index % ddgi.probeCounts.x;
    gridCoord.y = (index % (ddgi.probeCounts.x * ddgi.probeCounts.y)) / ddgi.probeCounts.x;
    gridCoord.z = index / (ddgi.probeCounts.x * ddgi.probeCounts.y);

    return gridToPosition(ddgi, gridCoord);
}

ivec3 probeIndexToGridCoord(in DDGIUniform ddgi, int index)
{
    ivec3 gridCoord;
    gridCoord.x = index % ddgi.probeCounts.x;
    gridCoord.y = (index % (ddgi.probeCounts.x * ddgi.probeCounts.y)) / ddgi.probeCounts.x;
    gridCoord.z = index / (ddgi.probeCounts.x * ddgi.probeCounts.y);
    return gridCoord;
}

ivec3 baseGridCoord(in DDGIUniform ddgi, vec3 X) 
{
    return clamp(ivec3((X - ddgi.startPosition.xyz) / ddgi.step.xyz), ivec3(0, 0, 0), ivec3(ddgi.probeCounts.xyz) - ivec3(1, 1, 1));
}

vec3 gridCoordToPosition(in DDGIUniform ddgi, ivec3 c)
{
    return ddgi.step.xyz * vec3(c) + ddgi.startPosition.xyz;
}

//Three dimension -> One dimension
int gridCoordToProbeIndex(in DDGIUniform ddgi, in ivec3 probeCoords) 
{
    return int(probeCoords.x + probeCoords.y * ddgi.probeCounts.x + probeCoords.z * ddgi.probeCounts.x * ddgi.probeCounts.y);
}

/**
* convert direction to texture coord from octahedron
* https://chengtsolin.files.wordpress.com/2015/03/octahedral1.jpg
* because of sampling proble was used bilinear interpolation
* 
* In order to ensure that the bilinear interpolation of the boundary is correct, 
* we need to extend a layer of outer boundary on the basis of the above texture storage, the method is as follows:
* 1.For the edge of the outer boundary, copy the original boundary upside down and fill it to the outer boundary
* 2.For the four corners of the outer border, use the pixel color of the opposite corner of the original border
*
* https://github.com/flwmxd/MapleEngine/blob/main/Screenshot/DDGIOct.png
*/
vec2 textureCoordFromDirection(vec3 dir, int probeIndex, int width, int height, int probeSideLength) 
{
    vec2 normalizedOctCoord = octEncode(normalize(dir));
    vec2 normalizedOctCoordZeroOne = (normalizedOctCoord + vec2(1.0f)) * 0.5f;

    // Length of a probe side, plus one pixel on each edge for the border
    float probeWithBorderSide = float(probeSideLength) + 2.0f;

    vec2 octCoordNormalizedToTextureDimensions = (normalizedOctCoordZeroOne * float(probeSideLength)) / vec2(float(width), float(height));

    int probesPerRow = (width - 2) / int(probeWithBorderSide); // how many probes in the texture altas

    // Add (2,2) back to texCoord within larger texture. Compensates for 1 pix 
    // border around texture and further 1 pix border around top left probe.
    vec2 probeTopLeftPosition = vec2(mod(probeIndex, probesPerRow) * probeWithBorderSide,
        (probeIndex / probesPerRow) * probeWithBorderSide) + vec2(2.0f, 2.0f);

    vec2 probeTopLeftPositionNormalized = vec2(probeTopLeftPosition) / vec2(float(width), float(height));

    return vec2(probeTopLeftPositionNormalized + octCoordNormalizedToTextureDimensions);
}


//P vertex.position
//N vertex.normal
vec3 sampleIrradiance(in DDGIUniform ddgi, vec3 P, vec3 N, vec3 Wo, sampler2D uIrradianceTexture, sampler2D uDepthTexture)
{
    ivec3 baseGridCoord = baseGridCoord(ddgi, P);
    vec3 baseProbePos   = gridCoordToPosition(ddgi, baseGridCoord);
    
    vec3  sumIrradiance = vec3(0.0f);
    float sumWeight = 0.0f;

    // alpha is how far from the floor(currentVertex) position. on [0, 1] for each axis.
    vec3 alpha = clamp((P - baseProbePos) / ddgi.step.xyz, vec3(0.0f), vec3(1.0f));

    for (int i = 0; i < 8; ++i) 
    {
        // Compute the offset grid coord and clamp to the probe grid boundary
        // Offset = 0 or 1 along each axis
        ivec3 offset = ivec3(i, i >> 1, i >> 2) & ivec3(1);
        ivec3 probeGridCoord = clamp(baseGridCoord + offset, ivec3(0), ddgi.probeCounts.xyz - ivec3(1));
        // Make cosine falloff in tangent plane with respect to the angle from the surface to the probe so that we never
        // test a probe that is *behind* the surface.
        // It doesn't have to be cosine, but that is efficient to compute and we must clip to the tangent plane.
        vec3 probePos = gridToPosition(ddgi, probeGridCoord);

        // Compute the trilinear weights based on the grid cell vertex to smoothly
        // transition between probes. Avoid ever going entirely to zero because that
        // will cause problems at the border probes. This isn't really a lerp. 
        // We're using 1-a when offset = 0 and a when offset = 1.
        vec3 trilinear = mix(1.0 - alpha, alpha, offset);
        float weight = 1.0;

        // Clamp all of the multiplies. We can't let the weight go to zero because then it would be 
        // possible for *all* weights to be equally low and get normalized
        // up to 1/n. We want to distinguish between weights that are 
        // low because of different factors.

        // Smooth backface test
        {
            // Computed without the biasing applied to the "dir" variable. 
            // This test can cause reflection-map looking errors in the image
            // (stuff looks shiny) if the transition is poor.
            vec3 dirToProbe = normalize(probePos - P);

            // The naive soft backface weight would ignore a probe when
            // it is behind the surface. That's good for walls. But for small details inside of a
            // room, the normals on the details might rule out all of the probes that have mutual
            // visibility to the point. So, we instead use a "wrap shading" test below inspired by
            // NPR work.
            // weight *= max(0.0001, dot(trueDirectionToProbe, wsN));

            // The small offset at the end reduces the "going to zero" impact
            // where this is really close to exactly opposite
            weight *= square(max(0.0001, (dot(dirToProbe, N) + 1.0) * 0.5)) + 0.2;
        }

        int probeIdx = gridCoordToProbeIndex(ddgi, probeGridCoord);


        // Moment visibility test
        {

            //  When d > u (mean), use the Chebyshev estimator as the Chebyshev weight; 
            //  when d < u, it is considered that the weight of the Chebyshev method is directly classified as 1 (there is no occluder between probe and shading point)

            //                  variance
            // P(r > d) <=  ----------------    
            //             variance + (d - mean)               
            // the r is the closest distance to object seen by the probe in the direction of the shading point
            // d is the distacne between probe and shading point

            // Bias the position at which visibility is computed; this
            // avoids performing a shadow test *at* a surface, which is a
            // dangerous location because that is exactly the line between
            // shadowed and unshadowed. If the normal bias is too small,
            // there will be light and dark leaks. If it is too large,
            // then samples can pass through thin occluders to the other
            // side (this can only happen if there are MULTIPLE occluders
            // near each other, a wall surface won't pass through itself.)
            vec3 vBias = (N + 3.0 * Wo) * ddgi.normalBias;
            vec3 probeToPoint = P - probePos + vBias;
            vec3 dir = normalize(-probeToPoint);

            vec2 texCoord = textureCoordFromDirection(-dir, probeIdx, ddgi.depthTextureWidth, ddgi.depthTextureHeight, ddgi.depthProbeSideLength);

            float dist = length(probeToPoint);

            vec2 temp = textureLod(uDepthTexture, texCoord, 0.0f).rg;
            float mean = temp.x;
            float variance = abs(square(temp.x) - temp.y);

            float chebyshevWeight = variance / (variance + square(max(dist - mean, 0.0)));
                
            // Increase contrast in the weight 
            chebyshevWeight = max(chebyshevWeight * chebyshevWeight * chebyshevWeight, 0.0);

            weight *= (dist <= mean) ? 1.0 : chebyshevWeight;
        }

        // Avoid zero weight
        weight = max(0.000001, weight);
                 
        vec3 irradianceDir = N;

        vec2 texCoord = textureCoordFromDirection(normalize(irradianceDir), probeIdx, ddgi.irradianceTextureWidth, ddgi.irradianceTextureHeight, ddgi.irradianceProbeSideLength);

        vec3 probeIrradiance = textureLod(uIrradianceTexture, texCoord, 0.0f).rgb;

        // A tiny bit of light is really visible due to log perception, so
        // crush tiny weights but keep the curve continuous. This must be done
        // before the trilinear weights, because those should be preserved.
        const float crushThreshold = 0.2f;
        if (weight < crushThreshold)
            weight *= weight * weight * (1.0f / square(crushThreshold)); 

        // Trilinear weights
        weight *= trilinear.x * trilinear.y * trilinear.z;

        // Weight in a more-perceptual brightness space instead of radiance space.
        // This softens the transitions between probes with respect to translation.
        // It makes little difference most of the time, but when there are radical transitions
        // between probes this helps soften the ramp.
        probeIrradiance = sqrt(probeIrradiance);
        sumIrradiance += weight * probeIrradiance;
        sumWeight += weight;
    }

    vec3 netIrradiance = sumIrradiance / sumWeight;
    
    netIrradiance.x = isnan(netIrradiance.x) ? 0.5f : netIrradiance.x;
    netIrradiance.y = isnan(netIrradiance.y) ? 0.5f : netIrradiance.y;
    netIrradiance.z = isnan(netIrradiance.z) ? 0.5f : netIrradiance.z;

    // Go back to linear irradiance
    netIrradiance = square(netIrradiance);
    netIrradiance *= ddgi.energyPreservation;

    return 0.5f * PI * netIrradiance;
}

#endif
