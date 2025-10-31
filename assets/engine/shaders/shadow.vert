#version 330 core

layout(location=0) in vec3 aPos;

// IMPORTANT: THIS IS NOT CAMERA MVP, BUT LIGHT MVP
// we project from the light's pov to see where the shadows should be
uniform mat4 u_M;
uniform mat4 u_V;
uniform mat4 u_P;

void main()
{
    gl_Position = u_P * u_V * u_M * vec4(aPos, 1.0); 
}