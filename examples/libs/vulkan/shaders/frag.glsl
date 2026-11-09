#version 450

layout(location = 0) in vec2 vUv;
layout(location = 1) in vec4 vColor;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D uTex;

void main()
{
    vec4 tex = texture(uTex, vUv);
    outColor = tex * vColor;
}