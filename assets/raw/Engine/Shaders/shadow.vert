#version 330 core

layout(location=0) in vec3 aPos;

uniform mat4 u_M;
uniform mat4 u_V;
uniform mat4 u_P;

void main()
{
    gl_Position = u_P * u_V * u_M * vec4(aPos, 1.0); 
}