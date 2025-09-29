// sphere.vert


#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;       // unused for sphere. doing this to prevent creating another VAO/VBO

out vec3 vFragPos;

uniform mat4 u_M;
uniform mat4 u_V;
uniform mat4 u_P;


void main() {
    vFragPos = vec3(u_M * vec4(aPos, 1.0));

    mat4 MVP = u_P * u_V * u_M;

    gl_Position = MVP * vec4(aPos, 1.0);
}
