#version 300 es

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

out vec3 vNormal;
out vec3 vFragPos;
out vec2 vTexCoords;

uniform mat4 u_M;
uniform mat4 u_V;
uniform mat4 u_P;

void main() {
    vNormal = mat3(transpose(inverse(u_M))) * aNormal;
    vFragPos = vec3(u_M * vec4(aPos, 1.0));
    vTexCoords = aTexCoords;

    mat4 MVP = u_P * u_V * u_M;
    gl_Position = MVP * vec4(aPos, 1.0);
}