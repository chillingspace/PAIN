// base.vert


#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

out vec3 vNormal;
out vec3 vFragPos;

uniform mat4 u_M;
uniform mat4 u_V;
uniform mat4 u_P;
uniform mat4 u_Model;

void main() {
    vNormal = mat3(transpose(inverse(u_Model))) * aNormal;
    vFragPos = vec3(u_Model * vec4(aPos, 1.0));

    mat4 MVP = u_P * u_V * u_M;

    gl_Position = MVP * vec4(aPos, 1.0);
}
