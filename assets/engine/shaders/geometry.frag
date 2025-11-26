// geometry.frag

#version 330 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_uniform_location : enable

#define PI 3.14159265359

layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec3 vFragPos;
layout(location = 2) in vec2 vTexCoords;
layout(location = 3) in mat3 TBN;

layout(location = 0) out vec3 gPos;
layout(location = 1) out vec3 gCol;
layout(location = 2) out vec3 gNorm;
layout(location = 3) out vec3 gMaterial;    // rough, metal, debugging_geometry
layout(location = 4) out vec3 gEmission;

struct Material {
    float rough;
    float metal;

    float useTex;
    sampler2D tex;
    float use_ao;
    sampler2D ao_map;
    float use_normal;
    sampler2D normal_map;
    float use_emission;
    sampler2D emission_map;
    float use_roughnessmetallic;
    sampler2D roughnessmetallic_map;

    vec3 color;         // fallback if there is no texture
    // float alwaysLit;    // bool
};



uniform Material material;

// debug
uniform float DEBUG_TYPE;

const int IBL_DEBUG_TYPE = 8;

void main() {
    // for debugging
    int dbg = int(DEBUG_TYPE);
    
    gPos = vFragPos;

    if (material.use_normal > 0.5) {
        vec3 normalMapContrib = texture(material.normal_map, vTexCoords).rgb;
        normalMapContrib = normalMapContrib * 2.0 - 1.0; // convert from [0,1] to [-1,1]
        
        // transform to world space
        gNorm = normalize(TBN * normalMapContrib);
    } else {
        // Use vertex normal if no normal map
        gNorm = normalize(vNormal);
    }

    // IBL_DEBUG_TYPE onwards is IBL stuff
    if (dbg > 0 && dbg < IBL_DEBUG_TYPE) {
        if (dbg == 1) gCol = vec3(1.0, 0.0, 1.0);
        else if (dbg == 2) gCol = texture(material.tex, vTexCoords).rgb;
        else if (dbg == 3) gCol = texture(material.ao_map, vTexCoords).rgb;
        else if (dbg == 4) gCol = texture(material.normal_map, vTexCoords).rgb;
        else if (dbg == 5) gCol = vec3(texture(material.roughnessmetallic_map, vTexCoords).g, 0, 0);
        else if (dbg == 6) gCol = vec3(texture(material.roughnessmetallic_map, vTexCoords).b, 0, 0);
        else if (dbg == 7) gCol = texture(material.emission_map, vTexCoords).rgb;

        return;
    }


    // gMaterial = vec3(material.rough, material.metal, material.alwaysLit);
    if (material.use_roughnessmetallic > 0.5) {
        gMaterial = vec3(texture(material.roughnessmetallic_map, vTexCoords).gb, dbg != 0 && dbg < IBL_DEBUG_TYPE ? 1.0 : 0.0);
    }
    else {
        gMaterial = vec3(material.rough, material.metal, dbg != 0 && dbg < IBL_DEBUG_TYPE ? 1.0 : 0.0);
    }

    if (material.use_emission > 0.5) {
        gEmission = texture(material.emission_map, vTexCoords).rgb;
    }
    else {
        gEmission = vec3(0.0, 0.0, 0.0);
    }


    if (material.useTex == 0.0) {
        gCol = material.color;
        return;
    }

    // use texture instead of fallback color
    // convert from sRGB to linear space
    vec3 color = pow(texture(material.tex, vTexCoords).rgb, vec3(2.2));

    // vec3 color = texture(material.tex, vTexCoords).rgb;

    if (material.use_ao == 0.0) {
        gCol = color;
        return;
    }

    // use ambient occlusion texture
    // also convert from sRGB to linear space
    float ao = pow(texture(material.ao_map, vTexCoords).r, 2.2);       // grayscale, just use red channel

    // float ao = texture(material.ao_map, vTexCoords).r;       // grayscale, just use red channel

    gCol = color * ao;
    // gCol = vec3(1,0,0);
}
