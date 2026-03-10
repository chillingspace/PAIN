#version 330 core

layout(location=0) in vec3 aPos;

// per-instance model matrix (instanced rendering)
layout(location = 7) in vec4 aInstanceM0;
layout(location = 8) in vec4 aInstanceM1;
layout(location = 9) in vec4 aInstanceM2;
layout(location = 10) in vec4 aInstanceM3;

// IMPORTANT: THIS IS NOT CAMERA MVP, BUT LIGHT MVP
// we project from the light's pov to see where the shadows should be
uniform mat4 u_M;
uniform mat4 u_V;
uniform mat4 u_P;
uniform float u_UseInstancing;

void main()
{
    mat4 effectiveM = u_UseInstancing > 0.5
        ? mat4(aInstanceM0, aInstanceM1, aInstanceM2, aInstanceM3)
        : u_M;
    gl_Position = u_P * u_V * effectiveM * vec4(aPos, 1.0);
}