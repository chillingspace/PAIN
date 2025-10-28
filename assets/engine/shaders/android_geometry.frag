#version 300 es
precision highp float;
precision highp sampler2D;

#define PI 3.14159265359

in vec3 vNormal;
in vec3 vFragPos;
in vec2 vTexCoords;

layout(location = 0) out vec3 gPos;
layout(location = 1) out vec3 gCol;
layout(location = 2) out vec3 gNorm;
layout(location = 3) out vec3 gMaterial;    // rough, metal, alwaysLit

struct Material {
    float rough;
    float metal;
    float useTex;
    vec3 color;         // fallback if there is no texture
    float alwaysLit;    // bool
    float use_ao;
};

uniform Material material;
uniform sampler2D material_tex;
uniform sampler2D material_ao_map;


void main() {
    gPos = vFragPos;
    gNorm = normalize(vNormal);
    gMaterial = vec3(material.rough, material.metal, material.alwaysLit);

    if (material.useTex == 0.0) {
        gCol = material.color;
        return;
    }

    // use texture instead of fallback color
    vec3 color = texture(material_tex, vTexCoords).rgb;

    if (material.use_ao == 0.0) {
        gCol = color;
        return;
    }

    // use ambient occlusion texture
    float ao = texture(material_ao_map, vTexCoords).r;       // grayscale, just use red channel
    gCol = color * ao;
}