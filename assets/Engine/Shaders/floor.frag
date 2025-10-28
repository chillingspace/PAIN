// floor.frag



#version 330 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_uniform_location : enable


layout(location=0) in vec3 fFragPos;

// layout(location = 0) out vec4 outColor;

layout(location = 0) out vec3 gPos;
layout(location = 1) out vec3 gCol;
layout(location = 2) out vec3 gNorm;
layout(location = 3) out vec3 gMaterial;


void main() {
    const float tile_size = 1.0;

    vec2 tile_coords = floor(fFragPos.xz / tile_size);

    float dark_tile_intensity = mod(tile_coords.x + tile_coords.y, 2.0);

    const vec3 dark = vec3(0.2, 0.2, 0.2);
    const vec3 light = vec3(0.5, 0.5, 0.5);

    // outColor = vec4(mix(light, dark, dark_tile_intensity), 1.0);

    const float roughness = 0.1;        // 1 -> rough, 0.1 -> smooth
    const float metallic = 0.3;         // metal

    gPos = fFragPos;
    gCol = mix(light, dark, dark_tile_intensity);
    gNorm = vec3(0.0, 1.0, 0.0);
    gMaterial = vec3(roughness, metallic, 0.0);
}
