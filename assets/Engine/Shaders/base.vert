#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec3 aNormal;

out vec3 vColor;
out vec3 vNormal;
out vec3 vFragPos;

uniform mat4 u_MVP;
uniform mat4 u_Model;

void main() {
    vColor = aColor;
    vNormal = mat3(transpose(inverse(u_Model))) * aNormal;
    vFragPos = vec3(u_Model * vec4(aPos, 1.0));

    gl_Position = u_MVP * vec4(aPos, 1.0);
}
