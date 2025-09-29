#version 330 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_uniform_location : enable

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

layout(location = 0) out vec3 vNormal;
layout(location = 1) out vec3 vFragPos;

layout(location = 0) uniform mat4 u_M;
layout(location = 1) uniform mat4 u_V;
layout(location = 2) uniform mat4 u_P;
layout(location = 3) uniform mat4 u_Model;

void main() {
    vNormal = mat3(transpose(inverse(u_Model))) * aNormal;
    vFragPos = vec3(u_Model * vec4(aPos, 1.0));

    mat4 MVP = u_P * u_V * u_M;
    gl_Position = MVP * vec4(aPos, 1.0);
}
