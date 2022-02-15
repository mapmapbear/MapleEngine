#version 450

layout (location = 0) in vec2 inCellIndex;

layout(set = 0, binding = 0) uniform UniformBufferObject
{
    int gridSize;
}ubo;

layout (location = 0) out ivec2 outCellIndex;

vec2 getGridOutputPosition()
{
    vec2 offsePosition = inCellIndex + vec2(0.5); //offset position to middle of texel
    float gridSize = float(ubo.gridSize);

    return vec2((2.0 * offsePosition.x) / (gridSize * gridSize), (2.0 * offsePosition.y) / gridSize) - vec2(1.0);
}

void main() 
{
    vec2 screenPos = getGridOutputPosition();

    outCellIndex = ivec2(inCellIndex);

    gl_PointSize = 1.0;
    gl_Position = vec4(screenPos, 0.0, 1.0);
}