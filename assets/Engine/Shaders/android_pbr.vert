#version 300 es

in vec3 aPos;
in vec3 aNormal;

out vec3 vNormal;
out vec3 vFragPos;

uniform mat4 u_M;
uniform mat4 u_V;
uniform mat4 u_P;

void main() {
    vNormal = mat3(transpose(inverse(u_M))) * aNormal;
    vFragPos = vec3(u_M * vec4(aPos, 1.0));

    mat4 MVP = u_P * u_V * u_M;
    gl_Position = MVP * vec4(aPos, 1.0);
}