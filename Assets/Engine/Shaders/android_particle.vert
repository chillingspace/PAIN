#version 300 es
precision highp float;
precision highp int;

// Per-vertex input (quad vertices)
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoords;

// Per-instance input
layout(location = 2) in vec3 aInstancePosition;
layout(location = 3) in vec4 aInstanceColor;
layout(location = 4) in float aInstanceSize;
layout(location = 5) in float aInstanceRotation;

out vec2 vTexCoords;
out vec4 vColor;

uniform mat4 u_V;
uniform mat4 u_P;

void main() {
    vTexCoords = aTexCoords;
    vColor = aInstanceColor;

    // Camera-facing billboard in view space
    vec4 viewCenter = u_V * vec4(aInstancePosition, 1.0);
    float c = cos(aInstanceRotation);
    float s = sin(aInstanceRotation);
    vec2 rotated = vec2(
        aPos.x * c - aPos.y * s,
        aPos.x * s + aPos.y * c
    );
    vec2 viewOffset = rotated * aInstanceSize;
    vec4 viewPos = vec4(viewCenter.xy + viewOffset, viewCenter.z, 1.0);
    gl_Position = u_P * viewPos;
}
