#version 330 core
layout (location = 0) in vec3 aPos; // Vertex position input

uniform mat4 u_VP; // Combined View * Projection matrix passed from C++

void main()
{
    // Transform vertex position to clip space
    gl_Position = u_VP * vec4(aPos, 1.0);
}