#version 300 es
precision highp float;

#define PI 3.14159265359

in vec3 vNormal;
in vec3 vFragPos;

layout(location = 0) out vec3 gPos;
layout(location = 1) out vec3 gCol;
layout(location = 2) out vec3 gNorm;
layout(location = 3) out vec2 gMaterial;    // rough, metal

struct Material {
    float rough;
    float metal;
    vec3 color;
};


uniform Material material;


void main() {
    gPos = vFragPos;
    gCol = material.color;
    gNorm = normalize(vNormal);
    gMaterial = vec2(material.rough, material.metal);
}