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
    // convert from sRGB to linear space
    vec3 color = pow(texture(material.tex, vTexCoords).rgb, vec3(2.2));

    if (material.use_ao == 0.0) {
        gCol = color;
        return;
    }

    // use ambient occlusion texture
    // also convert from sRGB to linear space
    float ao = pow(texture(material.ao_map, vTexCoords).r, 2.2);       // grayscale, just use red channel
    gCol = color * ao;
}