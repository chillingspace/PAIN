// occlusion.vert
// Minimal vertex shader for GPU occlusion queries.
// Transforms a unit-cube proxy to world space to test if an AABB is visible.

#version 330 core

layout(location = 0) in vec3 aPos;

uniform mat4 u_M; // scale+translate unit cube to worldAABB
uniform mat4 u_V;
uniform mat4 u_P;

void main() {
    gl_Position = u_P * u_V * u_M * vec4(aPos, 1.0);
}
