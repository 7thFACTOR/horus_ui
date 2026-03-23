#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inUv;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec2 vUv;
layout(location = 1) out vec4 vColor;

layout(push_constant) uniform Push { vec2 viewport; vec2 dummy; } pc;

void main()
{
    // convert pixel coords to NDC (assuming inPosition in pixels)
    vec2 ndc = (inPosition / pc.viewport) * 2.0 - 1.0;
    ndc.y = -ndc.y;
    gl_Position = vec4(ndc, 0.0, 1.0);
    vUv = inUv;
    vColor = inColor;
}