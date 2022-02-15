#version 450

#include "LPVCommon.glsl"

layout(location = 0) flat in ivec2 inCellIndex;

layout(set = 1, binding = 0) uniform UniformBufferObject
{
    int gridSize;
    bool firstIteration;
}ubo;

layout(set = 1, binding = 1) uniform sampler2D uRedSampler;
layout(set = 1, binding = 2) uniform sampler2D uGreenSampler;
layout(set = 1, binding = 3) uniform sampler2D uBlueSampler;

layout(set = 1, binding = 4) uniform sampler2D uRedGeometryVolume;
layout(set = 1, binding = 5) uniform sampler2D uGreenGeometryVolume;
layout(set = 1, binding = 6) uniform sampler2D uBlueGeometryVolume;


layout(location = 0) out vec4 outRedColor;
layout(location = 1) out vec4 outGreenColor;
layout(location = 2) out vec4 outBlueColor;

layout(location = 3) out vec4 outNextRedColor;
layout(location = 4) out vec4 outNextGreenColor;
layout(location = 5) out vec4 outNextBlueColor;

vec4 redContribution = vec4(0.0);
vec4 greenContribution = vec4(0.0);
vec4 blueContribution = vec4(0.0);
float occlusion_amplifier = 1.0f;

// orientation = [ right | up | forward ] = [ x | y | z ]
const mat3 neighbourOrientations[6] = mat3[] (
    // Z+
    mat3(
    1, 0, 0,
    0, 1, 0,
    0, 0, 1),
    // Z-
    mat3(
    -1, 0, 0,
    0, 1, 0,
    0, 0, -1),
    // X+
    mat3(
    0, 0, 1,
    0, 1, 0,
    -1, 0, 0),
    // X-
    mat3(
    0, 0, -1,
    0, 1, 0,
    1, 0, 0),
    // Y+
    mat3(
    1, 0, 0,
    0, 0, 1,
    0, -1, 0),
    // Y-
    mat3(
    1, 0, 0,
    0, 0, -1,
    0, 1, 0)
);

// Faces in cube
const ivec2 sideFaces[4] = ivec2[]
(
    ivec2(1, 0),   // right
    ivec2(0, 1),   // up
    ivec2(-1, 0),  // left
    ivec2(0, -1)   // down
);

vec3 getEvalSideDirection(int index, mat3 orientation)
{
    const float smallComponent = 0.4472135; // 1 / sqrt(5)
    const float bigComponent = 0.894427; // 2 / sqrt(5)

    vec2 currentSide = vec2(sideFaces[index]);
    return orientation * vec3(currentSide.x * smallComponent, currentSide.y * smallComponent, bigComponent);
}

vec3 getReprojSideDirection(int index, mat3 orientation)
{
    ivec2 currentSide = sideFaces[index];
    return orientation * vec3(currentSide.x, currentSide.y, 0);
}

void propagate()
{
    // Use solid angles to avoid inaccurate integral value stemming from low-order SH approximations
    const float directFaceSolidAngle = 0.4006696846f / PI;
	const float sideFaceSolidAngle = 0.4234413544f / PI;

    // Add contributions of neighbours to this cell
    for (int neighbour = 0; neighbour < 6; neighbour++)
    {
        mat3 orientation = neighbourOrientations[neighbour];
        vec3 direction = orientation * vec3(0.0, 0.0, 1.0);

        // Index offset in our flattened version of the lpv grid
        ivec2 indexOffset = ivec2(
            direction.x + (direction.z * float(ubo.gridSize)), 
            direction.y
        );

        ivec2 neighbourIndex = inCellIndex - indexOffset;

        vec4 redContributionNeighbour = texelFetch(uRedSampler, neighbourIndex, 0);
        vec4 greenContributionNeighbour = texelFetch(uGreenSampler, neighbourIndex, 0);
        vec4 blueContributionNeighbour = texelFetch(uBlueSampler, neighbourIndex, 0);

        // No occlusion
        float redOcclusionVal = 1.0;
        float greenOcclusionVal = 1.0;
        float blueOcclusionVal = 1.0;

        // No occlusion in the first step
        if (!ubo.firstIteration) {
            vec3 horizontalDirection = 0.5 * direction;
            ivec2 offset = ivec2(
                horizontalDirection.x + (horizontalDirection.z * float(ubo.gridSize)),
                horizontalDirection.y
            );
            ivec2 occ_coord = inCellIndex - offset;

            vec4 redOccCoeffs = texelFetch(uRedGeometryVolume, occ_coord, 0);
            vec4 greenOccCoeffs = texelFetch(uGreenGeometryVolume, occ_coord, 0);
            vec4 blueOccCoeffs = texelFetch(uBlueGeometryVolume, occ_coord, 0);

            redOcclusionVal = 1.0 - clamp(occlusion_amplifier * dot(redOccCoeffs, dirToSH(-direction)), 0.0, 1.0);
            greenOcclusionVal = 1.0 - clamp(occlusion_amplifier * dot(greenOccCoeffs, dirToSH(-direction)), 0.0, 1.0);
            blueOcclusionVal = 1.0 - clamp(occlusion_amplifier * dot(blueOccCoeffs, dirToSH(-direction)), 0.0, 1.0);
        }

        float occludedDirectFaceRedContribution = redOcclusionVal * directFaceSolidAngle;
        float occludedDirectFaceGreenContribution = greenOcclusionVal * directFaceSolidAngle;
        float occludedDirectFaceBlueContribution = blueOcclusionVal * directFaceSolidAngle;

        vec4 direction_cosine_lobe = evalCosineLobeToDir(direction);
        vec4 direction_spherical_harmonic = dirToSH(direction);

        redContribution += occludedDirectFaceRedContribution * max(0.0, dot(redContributionNeighbour, direction_spherical_harmonic)) * direction_cosine_lobe;
        greenContribution += occludedDirectFaceGreenContribution * max(0.0, dot( greenContributionNeighbour, direction_spherical_harmonic)) * direction_cosine_lobe;
        blueContribution += occludedDirectFaceBlueContribution * max(0.0, dot(blueContributionNeighbour, direction_spherical_harmonic)) * direction_cosine_lobe;

        // Add contributions of faces of neighbour
        for (int face = 0; face < 4; face++)
        {
            vec3 evalDirection = getEvalSideDirection(face, orientation);
            vec3 reprojDirection = getReprojSideDirection(face, orientation);

            // No occlusion in the first step
            if (!ubo.firstIteration) {
                vec3 horizontalDirection = 0.5 * direction;
                ivec2 offset = ivec2(
                    horizontalDirection.x + (horizontalDirection.z * float(ubo.gridSize)),
                    horizontalDirection.y
                );
                ivec2 occCoord = inCellIndex - offset;

                vec4 redOccCoeffs = texelFetch(uRedGeometryVolume, occCoord, 0);
                vec4 greenOccCoeffs = texelFetch(uGreenGeometryVolume, occCoord, 0);
                vec4 blueOccCoeffs = texelFetch(uBlueGeometryVolume, occCoord, 0);

                redOcclusionVal = 1.0 - clamp(occlusion_amplifier * dot(redOccCoeffs, dirToSH(-direction)), 0.0, 1.0);
                greenOcclusionVal = 1.0 - clamp(occlusion_amplifier * dot(greenOccCoeffs, dirToSH(-direction)), 0.0, 1.0);
                blueOcclusionVal = 1.0 - clamp(occlusion_amplifier * dot(blueOccCoeffs, dirToSH(-direction)), 0.0, 1.0);
            }

            float occludedSideFaceRedContribution = redOcclusionVal * sideFaceSolidAngle;
            float occludedSideFaceGreenContribution = greenOcclusionVal * sideFaceSolidAngle;
            float occludedSideFaceBlueContribution = blueOcclusionVal * sideFaceSolidAngle;

            vec4 reprojDirectionCosineLobe = evalCosineLobeToDir(reprojDirection);
			vec4 evalDirectionSphericalHarmonic = dirToSH(evalDirection);
			
		    redContribution += occludedSideFaceRedContribution * max(0.0, dot(redContributionNeighbour, evalDirectionSphericalHarmonic)) * reprojDirectionCosineLobe;
			greenContribution += occludedSideFaceGreenContribution * max(0.0, dot(greenContributionNeighbour, evalDirectionSphericalHarmonic)) * reprojDirectionCosineLobe;
			blueContribution += occludedSideFaceBlueContribution * max(0.0, dot(blueContributionNeighbour, evalDirectionSphericalHarmonic)) * reprojDirectionCosineLobe;
        }
    }
}

void main()
{
    propagate();

    outRedColor = redContribution;
    outGreenColor = greenContribution;
    outBlueColor = blueContribution;

    outNextRedColor = redContribution;
    outNextGreenColor = greenContribution;
    outNextBlueColor = blueContribution;
}