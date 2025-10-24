// geometry.frag

#version 330 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_uniform_location : enable

#define PI 3.14159265359

layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec3 vFragPos;

layout(location = 0) out vec3 gPos;
layout(location = 1) out vec3 gCol;
layout(location = 2) out vec3 gNorm;
layout(location = 3) out vec2 gMaterial;    // rough, metal

struct Material {
    float rough;
    float metal;
    float useTex;
    sampler2D tex;
    vec3 color;         // fallback if there is no texture
};


uniform Material material;


void main() {
    gPos = vFragPos;
    gNorm = normalize(vNormal);
    gMaterial = vec2(material.rough, material.metal);

    if (material.useTex == 0.0) {
        gCol = material.color;
        return;
    }

    // use texture instead of fallback color
    
}
