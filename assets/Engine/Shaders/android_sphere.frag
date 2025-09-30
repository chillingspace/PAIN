#version 300 es
precision highp float;

#define PI 3.14159265359

in vec3 vFragPos;

out vec4 FragColor;

uniform mat4 u_M;
uniform mat4 u_V;
uniform mat4 u_P;


void main() {
    FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    return;
}