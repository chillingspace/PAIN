// geometry.frag

#version 330 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_uniform_location : enable

#define PI 3.14159265359

layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec3 vFragPos;
layout(location = 2) in vec2 vTexCoords;

layout(location = 0) out vec3 gPos;
layout(location = 1) out vec3 gCol;
layout(location = 2) out vec3 gNorm;
layout(location = 3) out vec3 gMaterial;    // rough, metal, alwaysLit

struct Material {
    float rough;
    float metal;
    float useTex;
    sampler2D tex;
    vec3 color;         // fallback if there is no texture
    float alwaysLit;    // bool
    float use_ao;
    sampler2D ao_map;
};



uniform Material material;


void main() {
    gPos = vFragPos;
    gNorm = normalize(vNormal);
    gMaterial = vec3(material.rough, material.metal, material.alwaysLit);

    if (material.useTex == 0.0) {
        gCol = material.color;
        return;
    }

    // use texture instead of fallback color
    vec3 color = texture(material.tex, vTexCoords).rgb;

    if (material.use_ao == 0.0) {
        gCol = color;
        return;
    }

    // use ambient occlusion texture
    float ao = texture(material.ao_map, vTexCoords).r;       // grayscale, just use red channel
    gCol = color * ao;
    // gCol = vec3(1,0,0);
}
